program c_backend_allocate_stat_01
   implicit none

   integer :: stat
   integer, allocatable :: values(:)

   stat = 42
   allocate(values(3), stat=stat)
   if (stat /= 0) error stop

   values = [1, 2, 3]
   if (values(1) /= 1 .or. values(2) /= 2 .or. values(3) /= 3) error stop
end program c_backend_allocate_stat_01
