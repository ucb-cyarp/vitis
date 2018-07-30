function result = isFixedPointType(type)
%isFixedPointType Returns true if the given type string is a fixed
%point type.  Otherwise, returns false.

result = contains(type, 'fixdt') || contains(type, 'sfix') || contains(type, 'ufix');

end

