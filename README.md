# LARAGOLAS

GUI do laravel para Linux.

![Alt text](https://raw.githubusercontent.com/MarusDod/Laragolas/master/laragolas.png "laragolas")

## Requisitos
*   Ter uma Debian based distro
*   Systemd
*   Usar GNOME como DE

## Dependências
*   libgtk-3-dev
*   mysql
*   mysql-workbench
*   make
*   gcc
*   composer
*   php
*   apache2

## Instalação

```bash
   git clone https://github.com/MarusDod/Laragolas
   cd Laragolas
   chmod +x configure.sh
   ./configure.sh
   make release
   sudo make install
```

## Desinstalação

```bash
   sudo make uninstall
```

## Correr
```bash
   $ laragolas
```
