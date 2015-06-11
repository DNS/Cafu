local Vector3T_mt = {}


function Vector3T_mt.__add(a, b)
    return Vector3T(a[1] + b[1], a[2] + b[2], a[3] + b[3])
end


function Vector3T_mt.__sub(a, b)
    return Vector3T(a[1] - b[1], a[2] - b[2], a[3] - b[3])
end


function Vector3T_mt.__unm(a)
    return Vector3T(-a[1], -a[2], -a[3])
end


function Vector3T_mt.__mul(a, b)
    if getmetatable(a) == Vector3T_mt then
        return Vector3T(a[1] * b, a[2] * b, a[3] * b)
    else
        return Vector3T(b[1] * a, b[2] * a, b[3] * a)
    end
end


function Vector3T_mt.__div(a, b)
    if getmetatable(a) == Vector3T_mt then
        return Vector3T(a[1] / b, a[2] / b, a[3] / b)
    else
        return Vector3T(b[1] / a, b[2] / a, b[3] / a)
    end
end


-- Bitwise comparison on floats is barely useful...
function Vector3T_mt.__eq(a, b)
    return a[1] == b[1] and a[2] == b[2] and a[3] == b[3]
end


-- Alias "x", "y", "z" and "r", "g", "b" to 1, 2, 3.
function Vector3T_mt.__index(v, key)
    if key == "x" or key == "r" then return v[1] end
    if key == "y" or key == "g" then return v[2] end
    if key == "z" or key == "b" then return v[3] end

    return rawget(v, key)
end


-- Alias "x", "y", "z" and "r", "g", "b" to 1, 2, 3.
function Vector3T_mt.__newindex(v, key, value)
        if key == "x" or key == "r" then v[1] = value
    elseif key == "y" or key == "g" then v[2] = value
    elseif key == "z" or key == "b" then v[3] = value
    else
        rawset(v, key, value)
    end
end


function Vector3T_mt.__tostring(v)
    return "(" .. table.concat(v, ", ") .. ")"
end


-- It is possible to protect the metatable (PiL2, "13.3 Library-Defined Metamethods"),
-- but do we want this here?
-- Vector3T_mt.__metatable = "You cannot access the metatable of Vector3T instances."


-- The "constructor" function for creating new Vector3T instances.
function Vector3T(v, y, z)
    if type(v) == "number" then
        v = { v, y, z }
    end

    v = v or { 0.0, 0.0, 0.0 }
    setmetatable(v, Vector3T_mt)

    return v
end
