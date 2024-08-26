--[[verilua]]

local tbl = {1, 2, 3, 4}

local result = tbl.filter{ x => return x % 2 == 0 }

local function filter_func(x)
    return x % 2 == 0
end

local result2 = tbl.filter{filter_func}

result3 = tbl.zipWithIndex.filter{ (i, x) => 
    return i > 2 and x % 2 == 0
}

result4 = tbl.filter.zipWithIndex{ (x, i) => 
    return i > 2 and x % 2 == 0 
}

$comp_time {
    -- f = string.format
}

#define P(X) print(__LINE__, X)

P("hello")

local tbl = {1, 2, 3, 4, 5}
tbl.foreach{print}

tbl.zipWithIndex.foreach{
    (i, t) => print(i, t)
}

tbl2 = tbl.map{x => return x * 2}
tbl2.foreach{print}

tbl3 = tbl2.filter{x => return x % 4 == 0}
tbl3.foreach{print}

tbl4=tbl2.zipWithIndex.filter{
    (i, x) => 
    
        return i % 2 == 0 and x %4 == 0}
print()
tbl4.foreach{print}

$comp_time    {
    f = string.format
    ret = ""
    for i = 1, 3 do 
        ret = ret .. f("print(\"%s\", %d)\n", "from comp_time", i)
    end
    return ret
}


$comp_time{
    ret = ""
    for i = 1, 2 do 
        ret = ret .. f("print(\"%s\", %d)", "from 2comp_time", i)
    end
    return ret
}
