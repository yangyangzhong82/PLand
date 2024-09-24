# LDAPI

？> `LDAPI` 为 PLand 导出的 API  
LDAPI 的本质是一个宏，值为 `__declspec(dllimport)`  
头文件中，标记 LDAPI 的函数您都能访问、调用

## class 指南

?> 由于 PLand 提供了整个项目的导出，本文档无法实时更新，请以SDK为准

- `PLand`
    - 此类为PLand的核心类，负责存储、查询世界中的各个领地

- `LandData`
    - 此类为每个领地的数据，当您需要使用这个类时，请使用插件提供的 using `LandDataPtr`智能共享指针  
    - 注意：您不能直接释放 LandDataPtr，这会导致位置异常
