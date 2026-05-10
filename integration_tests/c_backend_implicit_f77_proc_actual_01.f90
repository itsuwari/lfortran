subroutine call_selector(selector, a, b, c, result)
    implicit none
    logical selector
    external selector
    double precision a, b, c
    logical result

    result = selector(a, b, c)
end subroutine

logical function selected(a, b, c)
    implicit none
    double precision a, b, c

    selected = a + b > c
end function

program c_backend_implicit_f77_proc_actual_01
    implicit none
    logical selected
    external selected
    logical result

    call call_selector(selected, 1.0d0, 2.0d0, 2.5d0, result)
    if (.not. result) error stop
end program
