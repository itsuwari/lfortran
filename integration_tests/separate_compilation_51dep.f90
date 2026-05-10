module separate_compilation_51dep
implicit none
private
public :: algorithm

type :: enum_algorithm
    integer :: gvd
    integer :: gvr
end type

type(enum_algorithm), parameter :: algorithm = enum_algorithm(1, 2)

end module
