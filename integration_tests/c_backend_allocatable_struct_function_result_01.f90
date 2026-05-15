module c_backend_allocatable_struct_function_result_01_mod
    implicit none

    type :: item
        integer :: value
    end type

contains

    function make_items(n) result(items)
        integer, intent(in) :: n
        type(item), allocatable :: items(:)
        integer :: i

        allocate(items(n))
        do i = 1, n
            items(i)%value = i * 2
        end do
    end function

end module

program c_backend_allocatable_struct_function_result_01
    use c_backend_allocatable_struct_function_result_01_mod, only: item, make_items
    implicit none

    type(item), allocatable :: values(:)

    values = make_items(3)

    if (size(values) /= 3) error stop "wrong result size"
    if (values(1)%value /= 2) error stop "wrong first value"
    if (values(2)%value /= 4) error stop "wrong second value"
    if (values(3)%value /= 6) error stop "wrong third value"
end program
