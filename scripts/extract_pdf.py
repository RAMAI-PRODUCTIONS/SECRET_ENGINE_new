#!/usr/bin/env python3
"""
Extract text from PDF for analysis
Requires: pip install PyPDF2
"""

try:
    from PyPDF2 import PdfReader
except ImportError:
    print("PyPDF2 not found. Installing...")
    import subprocess
    subprocess.check_call(['pip', 'install', 'PyPDF2'])
    from PyPDF2 import PdfReader

import os
from pathlib import Path

def extract_pdf_content(pdf_path, output_dir):
    """Extract text from PDF"""
    
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    text_output = output_dir / "extracted_text.md"
    
    print(f"Extracting from: {pdf_path}")
    
    reader = PdfReader(pdf_path)
    
    with open(text_output, 'w', encoding='utf-8') as f:
        f.write(f"# Extracted Content from {Path(pdf_path).name}\n\n")
        f.write(f"Total pages: {len(reader.pages)}\n\n")
        
        for page_num, page in enumerate(reader.pages, 1):
            print(f"Processing page {page_num}/{len(reader.pages)}")
            
            # Extract text
            text = page.extract_text()
            if text:
                f.write(f"\n## Page {page_num}\n\n")
                f.write(text)
                f.write("\n\n---\n")
    
    print(f"\nExtraction complete!")
    print(f"Text saved to: {text_output}")
    
    return text_output

if __name__ == "__main__":
    pdf_path = "docs/reference/82418-Joao-Monteiro-resumo-alargado.pdf"
    output_dir = "docs/reference/extracted"
    
    if os.path.exists(pdf_path):
        extract_pdf_content(pdf_path, output_dir)
    else:
        print(f"PDF not found: {pdf_path}")
