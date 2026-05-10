module c_backend_cleanup_dedupe_module_01
implicit none

type :: box
    real, allocatable :: values(:)
end type

contains

subroutine init_box(x)
    type(box), intent(out) :: x
    allocate(x%values(2))
    x%values = [1.0, 2.0]
end subroutine

end module
