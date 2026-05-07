module c_backend_alloc_char_deepcopy_01_m
implicit none

type :: child_type
    character(len=:), allocatable :: name
    integer :: value = 0
end type

type :: parent_type
    type(child_type), allocatable :: child
end type

contains

subroutine copy_parent(src, dst)
    type(parent_type), intent(in) :: src
    type(parent_type), intent(out) :: dst

    dst = src
end subroutine

end module

program c_backend_alloc_char_deepcopy_01
use c_backend_alloc_char_deepcopy_01_m, only: parent_type, copy_parent
implicit none

type(parent_type) :: src, dst

allocate(src%child)
src%child%value = 42

call copy_parent(src, dst)
if (.not. allocated(dst%child)) error stop
if (allocated(dst%child%name)) error stop
if (dst%child%value /= 42) error stop

src%child%name = "water"
call copy_parent(src, dst)
if (.not. allocated(dst%child%name)) error stop
if (dst%child%name /= "water") error stop

end program
