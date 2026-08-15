cmake_minimum_required(VERSION 3.16)

# Автоматически подключаем встроенные компоненты ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Регистрируем наш проект
project(robot-comm-secure)
