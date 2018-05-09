# SprotoJs
此库实现json数据打包为sproto协议格式二进制文件,同时可以解析其他语言数据格式(如lua)打包的sproto协议二进制文件为json格式的数据
# example:
协议文件为:
```
local bin = sprotoparser.parse [[  
.package {  
    type 0 : integer  
    session 1 : integer  
}

.Info  {
    money 0 : integer
    sex 1 : integer
}

TestReq 1 {
    request {
        addr 0: *string 
        info 1: Info
        age 2: *integer
        nClubID 3: integer
        name 4: string
    }
    response {
        addr 0: integer
    }
}

TestReq2 2 {
    request {
        age 0: integer
    }
}

]]  
调用:  
char* str  = "{"name":"shonm", "info":{"money":155, "sex":2}, "nClubID":500000, "age" : [11,22,33], "addr":["cn", "jp"]}"; 
char* retStr = sp.request("TestReq", str, &pMem, reserveLen, &retLen);  
解析: 
char* parsedJs = sp.dispatch(retStr, retLen, NULL, &retLen);   
```
打印结果如下:  
![des pic](https://github.com/shonm520/SprotoJs/blob/master/decode.jpg)

实现了数据的还原
