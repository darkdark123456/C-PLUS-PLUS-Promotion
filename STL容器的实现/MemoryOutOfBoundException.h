#include <iostream>

class MemoryOutOfBoundException
{
private:
    std::string errorMessage="Out of bounds exception";
public:
    MemoryOutOfBoundException() =default;
    ~MemoryOutOfBoundException()=default;
    std::string what() const {return errorMessage;}
};




