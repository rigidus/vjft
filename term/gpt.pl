#!/usr/bin/perl

use strict;
use warnings;

# Проверяем, передан ли путь к файлу через аргументы командной строки
die "Usage: $0 <source_file.c>\n" unless @ARGV == 1;

my $source_file = $ARGV[0]; # Имя файла исходного кода
my $output_file = "output.c"; # Имя выходного файла

open my $in, '<', $source_file or die "Cannot open '$source_file': $!";
open my $out, '>', $output_file or die "Cannot open '$output_file': $!";

while (my $line = <$in>) {
    # Проверяем, содержит ли строка директиву #include с локальным файлом
    if ($line =~ /^\s*#include "([^"]+)"\s*$/) {
        my $header_file = $1;
        print $out $line; # Выводим строку в выходной файл
        # Добавляем комментарий о начале файла
        print $out "// BEGIN_FILE($header_file)\n";

        if (-e $header_file) { # Проверяем существование файла
            open my $header, '<', $header_file or warn "Cannot open '$header_file': $!";
            while (my $header_line = <$header>) {
                print $out $header_line;
            }
            close $header;
        }

        # Добавляем комментарий о конце файла
        print $out "// END_FILE($header_file)\n";
    }
    print $out $line; # Выводим текущую строку кода в выходной файл
}

close $in;
close $out;

print "Processed '$source_file' to '$output_file'\n";
