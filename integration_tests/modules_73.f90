program modules_73
    use modules_73_mid, only: combined_value
    implicit none

    if (combined_value /= 42) error stop
    print *, "PASSED: modules_73"
end program modules_73
