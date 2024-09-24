# LDAPI

?> `LDAPI` 为 PLand 导出的 API  
`LDAPI` 的本质是一个宏，值为 `__declspec(dllimport)`  
头文件中，标记 `LDAPI` 的函数您都能访问、调用

!> ⚠️：您不能直接析构 `LandDataPtr` 以及其它智能指针，这会导致未知异常。  
您应该从持有此指针的 `class` 处调用对应函数进行释放

!> ⚠️：插件为了方便开发，部分class成员声明为public，但您不应该直接访问、修改这些成员，这会导致未知异常

## class 指南

?> 由于 PLand 提供了整个项目的导出，本文档无法实时更新，请以SDK为准

- `PLand`
  - 此类为PLand的核心类，负责存储、查询世界中的各个领地

- `LandData`
  - 此类为每个领地的数据，当您需要使用这个类时，请使用插件提供的 using `LandDataPtr`智能共享指针  
