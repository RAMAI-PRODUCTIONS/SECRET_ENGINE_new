#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/Fast/FastData.h>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine {
    
    // Camera Plugin - Single source of truth for camera state
    class CameraPlugin : public IPlugin {
    public:
        const char* GetName() const override { return "CameraPlugin"; }
        uint32_t GetVersion() const override { return 1; }
        
        void OnLoad(ICore* core) override {
            m_core = core;
            core->RegisterCapability("camera", this);
        }

        void OnActivate() override {}
        void OnDeactivate() override {}
        void OnUnload() override {}

        void* GetInterface(uint32_t id) override { return nullptr; }

        // Called by Core every frame
        void OnUpdate(float dt) override {
            // Load camera position from PlayerStart (once)
            if (!m_positionLoaded && m_core->GetWorld()) {
                m_positionLoaded = true;
            }
            
            // Consume input packets
            ProcessInputPackets();
            
            // Update movement
            UpdateMovement(dt);
            
            // Update camera matrices
            UpdateViewMatrix();
            
            // Send view matrix to renderer (internal cache)
            SendViewMatrixToRenderer();
        }

        // Set camera position (from scene loading)
        void SetPosition(float x, float y, float z) {
            m_pos[0] = x; m_pos[1] = y; m_pos[2] = z;
        }

        void SetRotation(float pitch, float yaw) {
            m_pitch = pitch; m_yaw = yaw;
        }

        // Get view-projection matrix (called by renderer)
        std::span<const float, 16> GetViewProjection() const { return m_viewProj; }

        std::span<const float, 3> GetPosition() const { return m_pos; }
        float GetYaw() const { return m_yaw; }
        float GetPitch() const { return m_pitch; }

    private:
        void ProcessInputPackets() {
            Fast::UltraPacket packet;
            auto* input = m_core->GetInput();
            if (!input) return;

            while (input->GetFastStream().Pop(packet)) {
                if (packet.GetType() == Fast::PacketType::InputAxis) {
                    if (packet.GetMetadata() == 1) {
                        // Look Input (Rotation)
                        float deltaX = static_cast<float>(packet.dataA) / 32767.0f;
                        float deltaY = static_cast<float>(packet.dataB) / 32767.0f;
                        
                        m_yaw += deltaX * m_lookSensitivity;
                        m_pitch -= deltaY * m_lookSensitivity;
                        
                        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
                    } else if (packet.GetMetadata() == 0) {
                        // Movement Input (Joystick)
                        m_moveJoyX = static_cast<float>(packet.dataA) / 32767.0f;
                        m_moveJoyY = static_cast<float>(packet.dataB) / 32767.0f;
                    }
                }
            }
        }

        void UpdateMovement(float dt) {
            // Calculate forward and right vectors from yaw (Z-up)
            float radYaw = m_yaw * 0.01745329f;
            float sinY = sinf(radYaw);
            float cosY = cosf(radYaw);

            // Forward vector (horizontally in XY plane for Z-up)
            float forward[3] = { sinY, -cosY, 0.0f }; // Z-up: movement in XY plane
            // Right vector
            float right[3] = { cosY, sinY, 0.0f }; // Z-up: right in XY plane

            // Apply joystick input
            float moveX = m_moveJoyX * m_moveSpeed * dt;
            float moveZ = -m_moveJoyY * m_moveSpeed * dt;
            
            m_pos[0] += forward[0] * moveZ + right[0] * moveX;
            m_pos[1] += forward[1] * moveZ + right[1] * moveX; // Z-up: Y is horizontal
        }

        void UpdateViewMatrix() {
            float radYaw = m_yaw * 0.01745329f;
            float radPitch = m_pitch * 0.01745329f;
            
            float cosP = cosf(radPitch);
            float sinP = sinf(radPitch);
            float cosY = cosf(radYaw);
            float sinY = sinf(radYaw);
            
            // Forward vector (Z-up: pitch affects Z component)
            float F[3] = {sinY * cosP, -cosY * cosP, sinP}; // Z-up coordinate system
            
            // Right vector (Z-up: right in XY plane)
            float R[3] = {cosY, sinY, 0.0f}; // Z-up: right vector on XY plane
            
            // Up vector (Z-up: cross product R x F)
            float U[3] = {
                R[1]*F[2] - R[2]*F[1],
                R[2]*F[0] - R[0]*F[2],
                R[0]*F[1] - R[1]*F[0]
            };
            
            // Build view matrix (column-major)
            m_view[0] = R[0];  m_view[4] = U[0];  m_view[8]  = -F[0];
            m_view[1] = R[1];  m_view[5] = U[1];  m_view[9]  = -F[1];
            m_view[2] = R[2];  m_view[6] = U[2];  m_view[10] = -F[2];
            m_view[3] = 0.0f;  m_view[7] = 0.0f;  m_view[11] = 0.0f;
            
            m_view[12] = -(R[0]*m_pos[0] + R[1]*m_pos[1] + R[2]*m_pos[2]);
            m_view[13] = -(U[0]*m_pos[0] + U[1]*m_pos[1] + U[2]*m_pos[2]);
            m_view[14] = -(-F[0]*m_pos[0] - F[1]*m_pos[1] - F[2]*m_pos[2]);
            m_view[15] = 1.0f;
        }

        void SendViewMatrixToRenderer() {
            // Calculate projection matrix
            float proj[16];
            float fov = 60.0f;
            float aspect = 1920.0f / 1080.0f; // Landscape default
            // aspect should technically come from renderer or core, but we'll stick to a reasonable default for now
            float f = 1.0f / tanf(fov * 0.5f * 0.01745329f);
            float n = 0.1f;
            float far = 5000.0f; // Increased far plane for randomized meshes
            
            memset(proj, 0, 16*sizeof(float));
            proj[0] = f / aspect;
            proj[5] = -f; // Vulkan flip Y
            proj[10] = far / (n - far);
            proj[11] = -1.0f;
            proj[14] = (n * far) / (n - far);
            
            // Multiply PV = P * V
            float pv[16];
            for(int c=0; c<4; ++c) {
                for(int r=0; r<4; ++r) {
                    pv[c*4 + r] = 
                        proj[r + 0*4] * m_view[c*4 + 0] +
                        proj[r + 1*4] * m_view[c*4 + 1] +
                        proj[r + 2*4] * m_view[c*4 + 2] +
                        proj[r + 3*4] * m_view[c*4 + 3];
                }
            }
            
            // Store for MegaGeometry to read
            memcpy(m_viewProj, pv, sizeof(pv));
        }

        ICore* m_core = nullptr;
        
        // Camera state (Z-up: X, Y horizontal, Z vertical)
        float m_pos[3] = {0.0f, 100.0f, 20.0f}; // Z-up: start at Z=20 height
        float m_yaw = 0.0f;
        float m_pitch = 0.0f;
        
        float m_lookSensitivity = 120.0f; // Fast rotation
        float m_moveSpeed = 150.0f;       // Fast movement
        
        float m_moveJoyX = 0.0f;
        float m_moveJoyY = 0.0f;
        
        bool m_positionLoaded = false;
        
        // View matrix
        float m_view[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        
        // View-Projection matrix (for renderer)
        float m_viewProj[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    };

    extern "C" IPlugin* CreateCameraPlugin() {
        return new CameraPlugin();
    }
}
