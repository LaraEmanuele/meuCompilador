@echo off
cd /d "%~dp0"

:: O gcc agora compila TODOS os arquivos .c juntos e ativa os alertas (-Wall -Wextra)
gcc -Wall -Wextra main.c lexer.c grammar.c parser.c -o output\main.exe

if %errorlevel% neq 0 (
    echo.
    echo [ERRO] Falha ao compilar o projeto.
    pause
    exit /b %errorlevel%
)

cls

echo ==================================================
echo           SAIDA DO MEU COMPILADOR
echo ==================================================

:: Executa o programa que está guardado lá dentro da output
output\main.exe

echo.
echo ==================================================
pause