// ============================================================================
// PNG TO ASTC CONVERTER - Command Line Tool
// Optimized batch texture compression for game assets
// ============================================================================

#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace SecretEngine::Tools {

// ============================================================================
// ASTC COMPRESSION SETTINGS
// ============================================================================
enum class ASTCQuality {
    FASTEST,      // -fast preset
    MEDIUM,       // -medium preset (default)
    THOROUGH,     // -thorough preset
    EXHAUSTIVE    // -exhaustive preset (very slow)
};

enum class ASTCBlockSize {
    BLOCK_4x4,    // 8.00 bpp - Highest quality
    BLOCK_5x5,    // 5.12 bpp
    BLOCK_6x6,    // 3.56 bpp
    BLOCK_8x8,    // 2.00 bpp - Best balance
    BLOCK_10x10,  // 1.28 bpp
    BLOCK_12x12   // 0.89 bpp - Lowest quality
};

struct ASTCConversionParams {
    ASTCQuality quality = ASTCQuality::MEDIUM;
    ASTCBlockSize blockSize = ASTCBlockSize::BLOCK_8x8;
    bool generateMipmaps = true;
    bool sRGB = true;
    int threadCount = -1;  // -1 = auto-detect
};

// ============================================================================
// ASTC CONVERTER
// ============================================================================
class ASTCConverter {
public:
    // Convert single PNG to ASTC
    static bool ConvertPNGToASTC(const char* inputPath, const char* outputPath, 
                                 const ASTCConversionParams& params = ASTCConversionParams());
    
    // Batch convert directory
    static bool ConvertDirectory(const char* inputDir, const char* outputDir, 
                                const ASTCConversionParams& params = ASTCConversionParams());
    
    // Get compression info
    struct CompressionInfo {
        uint32_t originalSize;
        uint32_t compressedSize;
        float compressionRatio;
        float compressionTimeMs;
    };
    
    static CompressionInfo GetLastCompressionInfo() { return s_lastInfo; }
    
private:
    static CompressionInfo s_lastInfo;
    
    static bool CompressPNGData(const unsigned char* pixels, uint32_t width, uint32_t height, 
                               std::vector<uint8_t>& outData, const ASTCConversionParams& params);
    
    static void GetBlockDimensions(ASTCBlockSize size, uint32_t& blockX, uint32_t& blockY);
};

// ============================================================================
// USAGE EXAMPLE
// ============================================================================
/*
// Convert single texture
ASTCConversionParams params;
params.blockSize = ASTCBlockSize::BLOCK_8x8;
params.quality = ASTCQuality::MEDIUM;

ASTCConverter::ConvertPNGToASTC("assets/textures/wall.png", 
                                "assets/textures/wall.astc", 
                                params);

// Batch convert all textures
ASTCConverter::ConvertDirectory("assets/textures", 
                               "assets/textures_astc", 
                               params);
*/

} // namespace SecretEngine::Tools
