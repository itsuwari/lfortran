module c_backend_component_array_cache_01_mod
implicit none

type :: box
    integer, allocatable :: id(:)
    real(8), allocatable :: xyz(:, :)
end type

contains

subroutine init_box(b)
type(box), intent(out) :: b

allocate(b%id(2), b%xyz(3, 2))
b%id(1) = 7
b%id(2) = 19
b%xyz(1, 1) = 2.0d0
b%xyz(2, 1) = 3.0d0
b%xyz(3, 1) = 5.0d0
b%xyz(1, 2) = 11.0d0
b%xyz(2, 2) = 13.0d0
b%xyz(3, 2) = 17.0d0
end subroutine

function score(b) result(total)
type(box), intent(in) :: b
real(8) :: total
integer :: i

total = 0.0d0
do i = 1, size(b%id)
    total = total + b%id(i)*b%xyz(1, i) + b%xyz(2, i) - b%xyz(3, i)
end do
end function

end module

program c_backend_component_array_cache_01
use c_backend_component_array_cache_01_mod, only: box, init_box, score
implicit none

type(box) :: b
real(8) :: total

call init_box(b)
total = score(b)
if (abs(total - 217.0d0) > 1.0d-12) error stop
print *, total
end program
