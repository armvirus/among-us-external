#include "stdafx.hpp"

#define local_player 0x5C
#define kill_cooldown 0x44
#define brightness 0x1c

namespace global_info
{
	HANDLE process_handle{};

	std::uint32_t process_id{};
	std::uintptr_t module_base{};

	std::uint32_t process_size{};
	std::uintptr_t copied_buffer{};
}

std::uint32_t dump_sig(const char* signature, int extra, int offset)
{
	std::uintptr_t instruction_offset = memory::signature_scan(global_info::copied_buffer, global_info::process_size, signature);

	return memory::read<std::uint32_t>(global_info::process_handle, global_info::module_base + instruction_offset + offset) + extra - global_info::module_base;
}

int main()
{
	std::printf("[+] starting [armvirus]\n\n");

	global_info::process_id = memory::find_process_id("Among Us.exe");

	if (!global_info::process_id)
	{
		printf("[-] failed to find process id\n");
		return -1;
	}

	std::printf("[+] process id [%i]\n", global_info::process_id);

	global_info::module_base = memory::get_module_base(global_info::process_id, "GameAssembly.dll");

	if (!global_info::module_base)
	{
		printf("[-] failed to find module base\n");
		return -1;
	}

	std::printf("[+] module base [%p]\n", global_info::module_base);

	global_info::process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, global_info::process_id);

	if (!global_info::process_handle)
	{
		printf("[-] failed to open handle to process [run as admin]\n");
		return -1;
	}

	std::printf("[+] handle to process [%p]\n\n", global_info::process_handle);

	scanner process_buffer(global_info::process_handle, global_info::process_id, global_info::module_base);

	std::tie(global_info::process_size, global_info::copied_buffer) = process_buffer.copy_process();

	std::printf("[+] process size [%i] > [%p]\n\n", global_info::process_size, global_info::copied_buffer);

	std::uint32_t main_offset = dump_sig("8b 0d ? ? ? ? 8b 5f ? f6 81 ? ? ? ? ? 74 ? 83 79 ? ? 75 ? 51 e8 ? ? ? ? 8b 45", 0, 2);

	printf("[+] dumped main offset [0x%x]\n", main_offset);

	printf("\n[+] press [F1] to close\n");

	while (!GetAsyncKeyState(VK_F1))
	{
		std::uint32_t offset_base = memory::read<std::uint32_t>(global_info::process_handle, global_info::module_base + main_offset);
		std::uint32_t local_player_object = memory::read<std::uint32_t>(global_info::process_handle, offset_base + local_player);
		std::uint32_t local_player_controller = memory::read<std::uint32_t>(global_info::process_handle, local_player_object);

		memory::write<float>(global_info::process_handle, local_player_controller + kill_cooldown, 0.0f);

		std::uint32_t shadow_manager = memory::read<std::uint32_t>(global_info::process_handle, local_player_controller + 0x54);

		memory::write<float>(global_info::process_handle, shadow_manager + brightness, 9999.0f);
	}

	return 0;
}
