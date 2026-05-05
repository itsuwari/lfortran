module c_backend_unlimited_poly_member_cleanup_01_m
implicit none

type :: payload_type
    integer, allocatable :: values(:)
end type

type :: cache_type
    class(*), allocatable :: raw
end type

contains

subroutine fill(cache)
    type(cache_type), intent(inout) :: cache
    type(payload_type), allocatable :: tmp

    allocate(tmp)
    allocate(tmp%values(2))
    tmp%values = [4, 9]
    call move_alloc(tmp, cache%raw)
end subroutine

subroutine run()
    type(cache_type) :: cache

    call fill(cache)
    if (.not. allocated(cache%raw)) error stop
    select type (raw => cache%raw)
    type is (payload_type)
        if (.not. allocated(raw%values)) error stop
        if (raw%values(2) /= 9) error stop
    class default
        error stop
    end select
end subroutine

subroutine early_return()
    type(cache_type), allocatable :: cache

    allocate(cache)
    call fill(cache)
    return
end subroutine

subroutine array_early_return()
    type(cache_type), allocatable :: caches(:)

    allocate(caches(1))
    call fill(caches(1))
    return
end subroutine

end module

program c_backend_unlimited_poly_member_cleanup_01
use c_backend_unlimited_poly_member_cleanup_01_m, only: array_early_return, early_return, run
implicit none

call run()
call early_return()
call array_early_return()
end program
