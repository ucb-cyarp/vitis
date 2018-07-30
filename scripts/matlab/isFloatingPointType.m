function result = isFloatingPointType(type)
%isFloatingPointType Returns true if the given type string is a floating
%point type.  Otherwise, returns false.

result = contains(type, 'single') || contains(type, 'double');
end

