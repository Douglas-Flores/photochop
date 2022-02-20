# Pré-requisitos
- GTK 3.2

## Instalando GTK 3.2
- Instruções para Windows (https://www.gtk.org/docs/installations/windows/):
- Instalar MSYS2 (https://www.msys2.org/);
- Install GTK3 and its dependencies. Open a MSYS2 shell, and run: pacman -S mingw-w64-x86_64-gtk3
- Install the GTK core applications: pacman -S mingw-w64-x86_64-glade
- Install the build tools: pacman -S mingw-w64-x86_64-toolchain base-devel

Caso seja necessário recompilar o código, rode no bash:
- gcc -Wall `pkg-config --cflags gtk+-3.0` -o photochop main.c `pkg-config --libs gtk+-3.0`