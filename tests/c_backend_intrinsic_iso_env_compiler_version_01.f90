program c_backend_intrinsic_iso_env_compiler_version_01
use, intrinsic :: iso_fortran_env, only: compiler_version
implicit none

character(:), allocatable :: version

version = compiler_version()
if (len(version) <= 0) error stop

end program c_backend_intrinsic_iso_env_compiler_version_01
