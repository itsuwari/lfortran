module class_148_m
    implicit none

    type, abstract :: term_type
    contains
        procedure(value_iface), deferred :: get_value
    end type

    abstract interface
        integer function value_iface(self)
            import :: term_type
            class(term_type), intent(in) :: self
        end function
    end interface

    type, extends(term_type) :: concrete_term
    contains
        procedure :: get_value => concrete_value
    end type

contains

    integer function concrete_value(self)
        class(concrete_term), intent(in) :: self
        concrete_value = 42
    end function

    subroutine check()
        class(term_type), allocatable :: term

        term = concrete_term()
        if (term%get_value() /= 42) error stop "dynamic dispatch lost"
    end subroutine

end module

program class_148
    use class_148_m, only: check
    implicit none

    call check()
end program
