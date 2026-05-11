module class_155_m
implicit none

type :: payload_type
    integer, allocatable :: values(:)
end type

type :: cache_type
    class(*), allocatable :: raw
end type

contains

subroutine fill(cache)
    type(cache_type), intent(out) :: cache
    type(payload_type), allocatable :: tmp

    allocate(tmp)
    allocate(tmp%values(1))
    tmp%values = [19]
    call move_alloc(tmp, cache%raw)
end subroutine

subroutine copy_cache(src, dest)
    type(cache_type), intent(in) :: src
    type(cache_type), intent(out) :: dest

    dest = src
end subroutine

subroutine check_payload(cache)
    type(cache_type), intent(in) :: cache

    if (.not. allocated(cache%raw)) error stop
    select type (raw => cache%raw)
    type is (payload_type)
        if (.not. allocated(raw%values)) error stop
        if (size(raw%values) /= 1) error stop
        if (raw%values(1) /= 19) error stop
    class default
        error stop
    end select
end subroutine

end module

program class_155
use class_155_m, only: cache_type, check_payload, copy_cache, fill
implicit none

type(cache_type) :: src
type(cache_type) :: dest

call fill(src)
call copy_cache(src, dest)
call check_payload(dest)
print *, "pass"
end program
