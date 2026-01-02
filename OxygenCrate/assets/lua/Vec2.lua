local Vec2 = {}
Vec2.__index = Vec2

function Vec2.new(x, y)
    return setmetatable({ x = x or 0.0, y = y or 0.0 }, Vec2)
end

function Vec2:clone()
    return Vec2.new(self.x, self.y)
end

function Vec2:add(other)
    return Vec2.new(self.x + other.x, self.y + other.y)
end

function Vec2:sub(other)
    return Vec2.new(self.x - other.x, self.y - other.y)
end

function Vec2:scale(scalar)
    return Vec2.new(self.x * scalar, self.y * scalar)
end

function Vec2:dot(other)
    return self.x * other.x + self.y * other.y
end

function Vec2:length()
    return math.sqrt(self:dot(self))
end

function Vec2:normalized()
    local len = self:length()
    if len <= 1e-6 then
        return Vec2.new(0.0, 0.0)
    end
    return self:scale(1.0 / len)
end

function Vec2:unpack()
    return self.x, self.y
end

function Vec2.__add(a, b)
    return Vec2.new(a.x + b.x, a.y + b.y)
end

function Vec2.__sub(a, b)
    return Vec2.new(a.x - b.x, a.y - b.y)
end

function Vec2.__mul(a, b)
    if type(a) == "number" then
        return Vec2.new(a * b.x, a * b.y)
    elseif type(b) == "number" then
        return Vec2.new(b * a.x, b * a.y)
    end
    return Vec2.new(a.x * b.x, a.y * b.y)
end

return Vec2
