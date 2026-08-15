# Регистрация приложения примера
idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES robot_secure_comm)
