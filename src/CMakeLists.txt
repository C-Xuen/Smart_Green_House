if(CONFIG_ENABLE_SMART_GREEN_HOUSE_SAMPLE)
    # 1. 添加所有需要编译的 .c 源文件
    set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SGH.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dht11.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LED.c

    )

    # 2. 添加头文件所在的目录
    set(PUBLIC_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()

set(SOURCES "${SOURCES}"  PARENT_SCOPE)
set(PUBLIC_HEADER "${PUBLIC_HEADER}" PARENT_SCOPE)