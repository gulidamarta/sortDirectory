# sortDirectory
### ОСиСП Лабораторная #2
Отсортировать в заданном каталоге (аргумент 1 командной строки) и во всех его
подкаталогах файлы по следующим критериям (аргумент 2 командной строки, задаётся в
виде целого числа):1 – по размеру файла, 2 – по имени файла. Записать без сохранения
структуры каталогов отсортированные файлы общим списком, в новый каталог (аргумент 3
командной строки). В связи с индексированием файлов в каталогах для файловых систем ext
2,3,4 перед запуском программы необходимо временно отключить опцию индексирования
файловой системы следующим образом:
#### sudo tune2fs –O ^dir_index /dev/sdaXY
Проверить результат, используя, ls -l –f.

#### P.S. 
Посоветовала бы очень аккуратно выполнять данную команду, так как у многих после её выполнения слетала Ubuntu.
