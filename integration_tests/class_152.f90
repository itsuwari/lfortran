module class_152_m
    implicit none

    integer :: deletes = 0

    type, abstract :: base_type
    contains
        procedure(delete_iface), deferred :: delete
    end type

    abstract interface
        subroutine delete_iface(self)
            import :: base_type
            class(base_type), intent(inout) :: self
        end subroutine
    end interface

    type, extends(base_type) :: child_type
    contains
        procedure :: delete => child_delete
    end type

contains

    subroutine child_delete(self)
        class(child_type), intent(inout) :: self
        deletes = deletes + 1
    end subroutine

    subroutine dispatch_allocatable_dummy(obj)
        class(base_type), allocatable, intent(inout) :: obj

        if (allocated(obj)) then
            call obj%delete()
        end if
    end subroutine

end module

program class_152
    use class_152_m, only: base_type, child_type, deletes, dispatch_allocatable_dummy
    implicit none

    class(base_type), allocatable :: obj

    allocate(child_type :: obj)
    call dispatch_allocatable_dummy(obj)
    if (deletes /= 1) error stop "allocatable dummy dispatch failed"
end program
