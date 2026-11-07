if {[catch {

# define run engine funtion
source [file join {C:/lscc/radiant/2026.1} scripts tcl flow run_engine.tcl]
# define global variables
global para
set para(gui_mode) "1"
set para(prj_dir) "C:/Users/pclark/Documents/GitHub/final-project/project_fpga/radiant_project/final_project"
if {![file exists {C:/Users/pclark/Documents/GitHub/final-project/project_fpga/radiant_project/final_project/impl_1}]} {
  file mkdir {C:/Users/pclark/Documents/GitHub/final-project/project_fpga/radiant_project/final_project/impl_1}
}
cd {C:/Users/pclark/Documents/GitHub/final-project/project_fpga/radiant_project/final_project/impl_1}
# synthesize IPs
# synthesize VMs
# propgate constraints
file delete -force -- final_project_impl_1_cpe.ldc
::radiant::runengine::run_engine_newmsg cpe -syn lse -f "final_project_impl_1.cprj" -a "iCE40UP"  -o final_project_impl_1_cpe.ldc
# synthesize top design
file delete -force -- final_project_impl_1.vm final_project_impl_1.ldc
::radiant::runengine::run_engine_newmsg synthesis -f "C:/Users/pclark/Documents/GitHub/final-project/project_fpga/radiant_project/final_project/impl_1/final_project_impl_1_lattice.synproj" -logfile "final_project_impl_1_lattice.srp"
::radiant::runengine::run_postsyn [list -a iCE40UP -p iCE40UP5K -t SG48 -sp High-Performance_1.2V -oc Industrial -top -w -o final_project_impl_1_syn.udb final_project_impl_1.vm] [list final_project_impl_1.ldc]

} out]} {
   ::radiant::runengine::runtime_log $out
   exit 1
}
