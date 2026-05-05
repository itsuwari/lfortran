program c_backend_block_allocatable_array_cleanup_ref_01
implicit none

call exercise()

contains

    subroutine fill(values)
        real, allocatable, intent(out) :: values(:, :, :)

        allocate(values(2, 3, 4), source=1.0)
    end subroutine

    subroutine check(values)
        real, intent(in) :: values(:, :, :)

        if (size(values) /= 24) error stop
        if (sum(values) /= 24.0) error stop
    end subroutine

    subroutine exercise()
        block
            real, allocatable :: local(:, :, :)

            call fill(local)
            call check(local)
        end block
    end subroutine

end program
