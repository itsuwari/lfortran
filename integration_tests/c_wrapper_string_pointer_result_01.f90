module c_wrapper_string_pointer_result_01
contains

    function cast_string() result(ptr)
        character(:), pointer :: ptr
        character(len=5), save, target :: tmp = "Hello"

        ptr => tmp
    end function cast_string

    function slice_ptr(s, i, j) result(out)
        character(len=*), intent(in), target :: s
        integer, intent(in) :: i, j
        character(:), pointer :: out

        out => s(max(i, 1):min(j, len(s)))
    end function slice_ptr

    subroutine test()
        character(:), pointer :: temp
        character(:), allocatable :: str

        temp => cast_string()
        str = temp

        if (str /= "Hello") error stop 1
        if (temp /= "Hello") error stop 2
        if (slice_ptr("ABCDE", 2, 4) /= "BCD") error stop 3
    end subroutine test

end module c_wrapper_string_pointer_result_01

program main
    use c_wrapper_string_pointer_result_01, only: test

    call test()
end program main
