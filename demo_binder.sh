#!/bin/bash

echo "=== Binder ����ģ����ʾ ==="
echo

echo "1. �����õ�exe�ļ���"
ls -la *.exe
echo

echo "2. ʹ��binder64.exe���������ļ���"
echo "����: wine ./binder64.exe stubloader64.exe bot.exe binder.exe demo_bundle.exe"
wine ./binder64.exe stubloader64.exe bot.exe binder.exe demo_bundle.exe
echo

echo "3. ������ɵ������ļ���"
if [ -f "demo_bundle.exe" ]; then
    echo "? �����ļ������ɹ���"
    echo "�ļ���Ϣ��"
    ls -la demo_bundle.exe
    file demo_bundle.exe
    echo
    
    echo "4. �ļ���С�Աȣ�"
    echo "ԭʼ�ļ���С��"
    echo "  - stubloader64.exe: $(stat -c%s stubloader64.exe) bytes"
    echo "  - bot.exe: $(stat -c%s bot.exe) bytes"  
    echo "  - binder.exe: $(stat -c%s binder.exe) bytes"
    echo "  - �ܼ�: $(($(stat -c%s stubloader64.exe) + $(stat -c%s bot.exe) + $(stat -c%s binder.exe))) bytes"
    echo "������ļ���С: $(stat -c%s demo_bundle.exe) bytes"
    echo
    
    echo "5. PE�ļ��ṹ������"
    echo "�����ļ����ڰ���10��������ԭstubloader��9������ + ������.bind������"
    echo
else
    echo "? �����ļ�����ʧ�ܣ�"
fi

echo "=== ��ʾ��� ==="
