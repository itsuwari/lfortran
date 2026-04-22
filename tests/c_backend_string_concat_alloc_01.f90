program c_backend_string_concat_alloc_01
    implicit none
    character(len=:), allocatable :: s

    s = "abc"
    s = s // "DEF"

    print "(a)", s
end program c_backend_string_concat_alloc_01
