module class_151_base
    implicit none

    type, abstract :: base_type
    contains
        procedure(run_interface), deferred :: run
    end type

    abstract interface
        subroutine run_interface(self, value)
            import :: base_type
            class(base_type), intent(inout) :: self
            integer, intent(out) :: value
        end subroutine
    end interface
end module

module class_151_mid
    use class_151_base, only: base_type
    implicit none

    type, abstract, extends(base_type) :: mid_type
    contains
        procedure :: run => mid_run
    end type

contains

    subroutine mid_run(self, value)
        class(mid_type), intent(inout) :: self
        integer, intent(out) :: value

        value = 151
    end subroutine
end module

module class_151_leaf
    use class_151_base, only: base_type
    use class_151_mid, only: mid_type
    implicit none

    type, extends(mid_type) :: leaf_type
    end type

contains

    subroutine make_leaf(obj)
        class(base_type), allocatable, intent(out) :: obj

        allocate(leaf_type :: obj)
    end subroutine
end module

program class_151
    use class_151_base, only: base_type
    use class_151_leaf, only: make_leaf
    implicit none

    class(base_type), allocatable :: obj
    integer :: value

    call make_leaf(obj)
    call obj%run(value)
    if (value /= 151) error stop "inherited deferred dispatch failed"
end program
