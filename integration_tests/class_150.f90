program class_150
    implicit none

    type :: cache_type
        class(*), allocatable :: raw
    end type

    type :: target_type
        integer :: value = 0
    end type

    type(cache_type), target :: cache
    type(target_type), pointer :: ptr

    allocate(integer :: cache%raw)
    call ensure_target(cache, ptr)
    if (.not. associated(ptr)) error stop "target pointer not associated"

    ptr%value = 42
    call ensure_target(cache, ptr)
    if (.not. associated(ptr)) error stop "target pointer lost"
    if (ptr%value /= 42) error stop "target value lost"

contains

    subroutine ensure_target(cache, ptr)
        type(cache_type), target, intent(inout) :: cache
        type(target_type), pointer, intent(out) :: ptr

        nullify(ptr)
        if (allocated(cache%raw)) then
            select type (target => cache%raw)
            type is (target_type)
                ptr => target
            end select
            if (associated(ptr)) return
            deallocate(cache%raw)
        end if

        if (.not. allocated(cache%raw)) then
            block
                type(target_type), allocatable :: tmp
                allocate(tmp)
                call move_alloc(tmp, cache%raw)
            end block
        end if

        select type (target => cache%raw)
        type is (target_type)
            ptr => target
        end select
    end subroutine

end program
