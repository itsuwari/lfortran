program c_backend_string_concat_alloc_01
    implicit none
    character(len=:), allocatable :: s

    s = "abc"
    s = s // "DEF"

    print "(a)", s

    s = "Interactions"
    s = s // " (1)"
    s = s // achar(10) // " | [1] spin polarization"
    if (s /= "Interactions (1)" // achar(10) // " | [1] spin polarization") then
        error stop "bad concat null termination"
    end if
end program c_backend_string_concat_alloc_01
