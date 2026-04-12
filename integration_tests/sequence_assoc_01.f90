program sequence_assoc_01
    implicit none
    real :: a(5)

    a = 0.0
    call fill_tail(a(2))

    if (a(1) /= 0.0) error stop "sequence_assoc_01: prefix modified"
    if (a(2) /= 10.0) error stop "sequence_assoc_01: element 2 mismatch"
    if (a(3) /= 20.0) error stop "sequence_assoc_01: element 3 mismatch"
    if (a(4) /= 30.0) error stop "sequence_assoc_01: element 4 mismatch"
    if (a(5) /= 0.0) error stop "sequence_assoc_01: suffix modified"

    print *, "sequence_assoc_01 ok"
contains
    subroutine fill_tail(x)
        real, intent(inout) :: x(*)
        x(1) = 10.0
        x(2) = 20.0
        x(3) = 30.0
    end subroutine fill_tail
end program sequence_assoc_01
