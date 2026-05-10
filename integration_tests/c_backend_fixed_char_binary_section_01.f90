program c_backend_fixed_char_binary_section_01
   use iso_fortran_env, only : int16, int32
   implicit none

   character(len=*), parameter :: filename = "c_backend_fixed_char_binary_section_01.bin"
   character(len=*), parameter :: target = "tblite0_kt"
   integer :: io, stat
   character(len=30) :: local_header
   character(len=:), allocatable :: path
   character(len=1), allocatable :: buffer(:)
   logical :: found
   integer(int32) :: nbytes

   open(newunit=io, file=filename, form="unformatted", access="stream", &
      status="replace", action="write")
   call write_record(io, "dummy.npy", "abc")
   call write_record(io, "tblite0_kt.npy", "xyz")
   close(io)

   found = .false.
   open(newunit=io, file=filename, form="unformatted", access="stream", &
      status="old", action="read", iostat=stat)
   if (stat /= 0) error stop "open failed"

   do while (stat == 0)
      read(io, iostat=stat) local_header
      if (stat /= 0) exit
      if (.not. is_local_header(local_header)) exit

      call read_data(io, local_header, path, nbytes, stat)
      if (stat /= 0) exit
      if (allocated(buffer)) deallocate(buffer)
      allocate(buffer(nbytes))
      read(io, iostat=stat) buffer
      if (stat /= 0) exit

      if (target//".npy" == path) then
         found = .true.
         exit
      end if
   end do
   close(io, status="delete")

   if (.not. found) error stop "target not found"

contains

   subroutine write_record(io, name, payload)
      integer, intent(in) :: io
      character(len=*), intent(in) :: name
      character(len=*), intent(in) :: payload
      character(len=30) :: header
      integer(int16) :: extra
      integer(int32) :: sig, nbytes_local

      header = achar(0)
      sig = int(z'04034b50', int32)
      nbytes_local = len(payload, kind=int32)
      extra = 0_int16
      header(1:4) = transfer(sig, header(1:4))
      header(19:22) = transfer(nbytes_local, header(19:22))
      header(27:28) = transfer(int(len(name), int16), header(27:28))
      header(29:30) = transfer(extra, header(29:30))
      write(io) header
      write(io) name
      write(io) payload
   end subroutine write_record

   subroutine read_data(io, local_header, path, nbytes, stat)
      integer, intent(in) :: io
      character(len=30), intent(in) :: local_header
      character(len=:), allocatable, intent(out) :: path
      integer(int32), intent(out) :: nbytes
      integer, intent(out) :: stat

      integer(int16) :: path_size, extra_field_size
      character(len=:), allocatable :: extra

      stat = 0
      path_size = transfer(local_header(27:28), path_size)
      extra_field_size = transfer(local_header(29:30), extra_field_size)
      nbytes = transfer(local_header(19:22), nbytes)
      allocate(character(len=path_size) :: path, stat=stat)
      if (stat == 0) read(io, iostat=stat) path
      if (stat == 0 .and. extra_field_size > 0) then
         allocate(character(len=extra_field_size) :: extra, stat=stat)
         if (stat == 0) read(io, iostat=stat) extra
      end if
   end subroutine read_data

   logical function is_local_header(local_header)
      character(len=*), intent(in) :: local_header
      integer(int32) :: header_sig
      header_sig = transfer(local_header(1:4), header_sig)
      is_local_header = header_sig == int(z'04034b50', int32)
   end function is_local_header

end program c_backend_fixed_char_binary_section_01
