#!/bin/bash

# Список файлов
files=("Main.cpp" "App.hpp" "App.cpp" "Event.hpp" "EventListener.hpp" "EventManager.hpp" "KeyEvent.hpp" "Makefile" "Player.hpp" "SpriteSheet.hpp" "SpriteSheet.cpp" "StickFigure.hpp" "StickFigure.cpp" "TextRenderer.hpp" "TextRenderer.cpp" "Utils.hpp" "Utils.cpp" "StackCleanup.hpp" "StackCleanup.cpp")

# Имя выходного файла
output_file="output.md"

# Очистка выходного файла, если он существует
> "$output_file"

# Проход по каждому файлу в списке
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "Processing $file"
        echo '```' >> "$output_file"
        cat "$file" >> "$output_file"
        echo '```' >> "$output_file"
        echo "" >> "$output_file"  # добавление пустой строки между файлами для читабельности
    else
        echo "File $file does not exist, skipping..."
    fi
done

echo "Files have been processed and saved to $output_file"
