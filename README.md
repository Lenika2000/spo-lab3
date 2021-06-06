# Лабораторная работа № 3 по СПО;

## Описание работы

Разработать клиент-серверное приложение.
Для организации взаимодействия по сети, поддержки множества соединений использовать программные интерфейсы (API) операционной системы.

Сервер и клиент взаимодействуют по протоколу, реализованному на базе сокетов (использовать TCP, если в варианте задания не указано обратное).
Сервер должен поддерживать условно неограниченное количество клиентов.
На всех этапах взаимодействия клиента и сервера должна быть предусмотрена обработка данных, независимо от их размера.

Порядок выполнения:
1. Выполнить анализ предметной области, которая задается вариантом к лабораторной работе.
   Результатом анализа должен быть набор сущностей, которые будут использоваться на уровне типов данных (структур, операций) при реализации программы.
1. Составить диаграмму, на которой схематически будут показаны результаты анализа:
   сущности, их атрибуты и взаимосвязи.
1. Составить план постепенного выполнения задания:
   какие части функциональности, в каком порядке предполагается реализовывать, в каком порядке и как проверять их работоспособность.
1. Загрузить все полученные артефакты в отдельную директорию &laquo;docs&raquo; репозитория, в корневую директорию положить `README.md` с номером варианта и коротким описанием.
1. Продемонстрировать составленные диаграмму и план преподавателю.
   Для этого достаточно просто отправить преподавателю ссылку на репозиторий.
1. После проверки и получения рекомендаций приступить к реализации программы, создавая на каждый этап выполнения отдельную ветку в репозитории.
1. По завершении каждого этапа создать pull-request, включив преподавателя в число reviewer-ов.
1. Если pull-request отклоняется преподавателем, выполнить необходимые правки и обновить его, запросив повторное ревью.
1. Когда pull-request одобрен, &laquo;слить&raquo; его с основной веткой кода, после чего создать новую ветку для работы над следующим этапом.

## Чат

Программа может выполняться в двух режимах: сервер или клиент.
Режим определяется аргументом командной строки.

В режиме сервера линейно отображается журнал всех входящих и исходящих сообщений, завершение программы-сервера выполняется по нажатию ключевой клавиши (например, Q).

При запуске в режиме клиента программе в качестве аргументов командной строки также передается имя пользователя и адрес сервера.
Необходимо предусмотреть возможность отправки &laquo;приватного&raquo; сообщения, которое увидит только адресат, которому оно предназначено.
При подключении отображать последние 20 сообщений, предусмотреть возможность просмотра истории сообщений (не сохраняя ее при этом на стороне клиентского приложения).

Новые сообщения выводятся в конце списка с автопрокруткой по мере появления новых сообщений.
Клавишами &uarr;, &darr;, Page Up, Page Down выполняется прокрутка списка сообщений, отменяющая автопрокрутку при появлении новых сообщений.
Клавишей End автопрокрутка возобновляется.
Собственное сообщение вводится в отдельной строке в нижней части окна, не блокируя историю сообщений.

Запуск 
sudo ./main s port - сервер
sudo ./main с ClientID localhost(127.0.0.1) port - клиент

отправка приватного сообщения private receiver msg