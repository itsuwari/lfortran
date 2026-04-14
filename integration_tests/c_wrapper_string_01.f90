module c_wrapper_string_01
contains

    subroutine take_sym(sym)
        character(len=*), intent(in) :: sym(:)

        if (size(sym) /= 2) error stop 1
        if (sym(1) /= "ab") error stop 2
        if (sym(2) /= "cd") error stop 3
    end subroutine

    subroutine test()
        character(len=:), allocatable :: sym(:)

        allocate(character(len=2) :: sym(2))
        sym = ["ab", "cd"]
        call take_sym(sym)
    end subroutine

end module

program main
    use c_wrapper_string_01

    call test()
end program
