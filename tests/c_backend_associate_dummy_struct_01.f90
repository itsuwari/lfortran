module c_backend_associate_dummy_struct_01_m
implicit none

type :: box
    integer :: value = 0
end type

contains

subroutine init_box(self)
    type(box), intent(out) :: self
    associate(alias => self)
        alias%value = 42
    end associate
end subroutine

end module

program c_backend_associate_dummy_struct_01
use c_backend_associate_dummy_struct_01_m, only: box, init_box
implicit none
type(box) :: item

call init_box(item)
if (item%value /= 42) error stop
end program
