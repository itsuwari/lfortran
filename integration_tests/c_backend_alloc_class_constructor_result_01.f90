program c_backend_alloc_class_constructor_result_01
implicit none

type :: base
    integer :: tag = 0
end type

type, extends(base) :: child
    character(len=:), allocatable :: label
    real(8) :: values(3) = 0.0d0
end type

class(base), allocatable :: cont
real(8) :: v(3)

v = [1.0d0, 2.0d0, 3.0d0]
cont = make_child(v)

select type (cont)
type is (child)
    if (cont%tag /= 7) error stop
    if (.not. allocated(cont%label)) error stop
    if (cont%label /= "child") error stop
    if (any(cont%values /= v)) error stop
class default
    error stop
end select

contains

function make_child(v) result(new)
real(8), intent(in) :: v(:)
type(child) :: new

new%tag = 7
new%label = "child"
new%values = v(:3)
end function

end program
