<h1 style="font-size: 28px; color: #2c3e50; text-align: center;">
    🔌 ПО ДЛЯ ИЗМЕРИТЕЛЬНЫХ ПРИБОРОВ [РЕАЛИЗАЦИЯ USB TMC + ПАРСЕР SCPI]
</h1>

---

# 📌 Краткое описание:
#### Реализация протокола USB TMC (Test & Measurement Class) для управления измерительными устройствами с поддержкой команд SCPI (Standard Commands for Programmable Instruments). Проект включает:

#### ✅ USB-стек с обработкой дескрипторов и транзакций

#### ✅ Механизм потоковой передачи данных - [ Поддержка Bulk IN/OUT конечных точек ]

#### ✅ Парсер SCPI-команд с иерархической обработкой.

#### ✅ Хранение идентификаторов в энергонезависимой памяти.

---

# ⚙️ Общая логика приема приема/обработки данных по USB:
![image](https://github.com/user-attachments/assets/adfd972e-a1de-4b93-b256-0f25a605873b)



# ⚙️ SCPI Парсер/логика добавления новых классов:
![image](https://github.com/user-attachments/assets/b474c928-91bc-4356-b68d-bdc434bf6ea1)

---

# ⚠️ Ограничения:

#### ❌ Реализация представлена исключительно для демонстрации архитектурных решений и не является полной или рабочей. 

#### ❌ Отсутствует поддержка аппаратно-зависимых модулей.

#### ❌ Вырезаны адаптеры устройств и модули SCPI классов.

#### ❌ Тестовые команды SCPI ограничены набором *IDN?

#### ❌ Упрощена обработка ошибок USB-транзакций.

---

# ⚙️ Ссылки:
#### Спецификация USB класса TMC: https://www.usb.org/document-library/test-measurement-class-specification
#### SCPI протокол: https://en.wikipedia.org/wiki/Standard_Commands_for_Programmable_Instruments
