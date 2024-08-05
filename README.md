# Bus_manager
Система слежения за графиком движения автобусов по маршруту.

Проект разрабатывался в целях изучения взаимодействия потоков в многопоточных приложениях.

Программа представляет собой оконное Windows приложение, написаное на С++. Для создания и управления окном, управления потоками, отрисовки графики использовались инструменты WinAPI. 

Интерфейс:

<img alt="UI" src="https://github.com/user-attachments/assets/825795fa-0a35-4c5d-b996-7970868eb528" width="600"/>

---

В списке слева можно выбрать автобус, для которого будет отображаться информация в полях слева от кнопок управления.

В списке справа можно выбрать станцию, для которой будет отображаться информация в полях справа от кнопок управления.

Движение каждого автобуса моделируется отдельным потоком. Скорость движения каждого автобуса между остановками считается постоянной, однако на каждом сегменте она принимает новое значение. Программа (основной поток) отображает информацию по выбранному автобусу и для выбранной остановки. С помощью кнопок на экране можно добавлять и удалять автобусы, а также управлять выбранным автобусом или всеми сразу.
Для отрисовки графики в прямоугольнике сверху используется отдельный поток и набор разработанных мной функций использующих возможности WinAPI.

