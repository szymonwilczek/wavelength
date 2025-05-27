#!/bin/bash

echo "Konwersja wszystkich plików SVG do PNG..."

if ! command -v inkscape &> /dev/null; then
    echo "Instalowanie inkscape..."
    sudo pacman -S inkscape
fi

mkdir -p diagrams-export

if [ ! -d "docs/html" ]; then
    echo "Błąd: Katalog docs/html nie istnieje!"
    echo "Upewnij się, że uruchomiłeś wcześniej: doxygen Doxyfile"
    exit 1
fi

counter=0

find docs/html -name "*.svg" -type f | while read svg_file; do
    filename=$(basename "$svg_file" .svg)

  inkscape "$svg_file" --export-type=png \
      --export-filename="diagrams-export/${filename}.png" \
      --export-dpi=600 \
      --export-background=white \
      --export-background-opacity=1.0

    if [ $? -eq 0 ]; then
        echo "✓ Skonwertowano: ${filename}.png"
        ((counter++))
    else
        echo "✗ Błąd przy konwersji: ${filename}.svg"
    fi
done

echo ""
echo "=========================================="
echo "Konwersja zakończona!"
echo "=========================================="
echo "Wszystkie diagramy znajdują się w folderze: diagrams-export/"
echo ""

if [ -d "diagrams-export" ] && [ "$(ls -A diagrams-export 2>/dev/null)" ]; then
    echo "Wygenerowane pliki PNG:"
    ls -la diagrams-export/*.png 2>/dev/null | awk '{printf "  %-40s %s\n", $9, $5}' | sed 's|diagrams-export/||' | sed 's|\.png||'
    echo ""

    count=$(ls diagrams-export/*.png 2>/dev/null | wc -l)
    echo "Łącznie wygenerowano: $count plików PNG"

    size=$(du -sh diagrams-export 2>/dev/null | cut -f1)
    echo "Całkowity rozmiar: $size"
else
    echo "Nie znaleziono żadnych plików SVG do konwersji."
fi

echo ""
echo "Gotowe! 🎉"
