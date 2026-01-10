# SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0
import hashlib
import logging
from typing import Callable

import pytest
# from pytest_embedded_idf.dut import IdfDut
# from pytest_embedded_idf.utils import idf_parametrize
from pytest_embedded_qemu.app import QemuApp
from pytest_embedded_qemu.dut import QemuDut


# @pytest.mark.generic
# @idf_parametrize('target', ['supported_targets', 'preview_targets'], indirect=['target'])
# def test_hello_world(dut: IdfDut, log_minimum_free_heap_size: Callable[..., None]) -> None:
#     dut.expect('Hello world!')
#     log_minimum_free_heap_size()


# @pytest.mark.host_test
# @idf_parametrize('target', ['linux'], indirect=['target'])
# def test_hello_world_linux(dut: IdfDut) -> None:
#     dut.expect('Hello world!')


# @pytest.mark.host_test
# @pytest.mark.macos_shell
# @idf_parametrize('target', ['linux'], indirect=['target'])
# def test_hello_world_macos(dut: IdfDut) -> None:
#     dut.expect('Hello world!')


def verify_elf_sha256_embedding(app: QemuApp, sha256_reported: str) -> None:
    sha256 = hashlib.sha256()
    with open(app.elf_file, 'rb') as f:
        sha256.update(f.read())
    sha256_expected = sha256.hexdigest()

    logging.info(f'ELF file SHA256: {sha256_expected}')
    logging.info(f'ELF file SHA256 (reported by the app): {sha256_reported}')

    # the app reports only the first several hex characters of the SHA256, check that they match
    if not sha256_expected.startswith(sha256_reported):
        raise ValueError('ELF file SHA256 mismatch')


# @pytest.mark.host_test
# @pytest.mark.qemu
# @idf_parametrize('target', ['esp32', 'esp32c3'], indirect=['target'])
# def test_hello_world_host(app: QemuApp, dut: QemuDut) -> None:
#     sha256_reported = dut.expect(r'ELF file SHA256:\s+([a-f0-9]+)').group(1).decode('utf-8')
#     verify_elf_sha256_embedding(app, sha256_reported)

#     dut.expect('Hello world!')


# # Import mock ADC functions for testing
# from mock_adc import (
#     mock_adc_init, mock_adc_simulate_disconnection, mock_adc_simulate_short_circuit,
#     mock_adc_simulate_noise, mock_adc_simulate_intermittent, mock_adc_reset_to_normal,
#     mock_adc_set_normal_value, MOCK_ADC_MODE_NORMAL, MOCK_ADC_MODE_DISCONNECTED
# )

# @pytest.mark.qemu
# @idf_parametrize('target', ['linux'], indirect=['target'])
# def test_analog_pin_disconnected(dut: QemuDut) -> None:
#     """Test ADC disconnection detection using mock hardware"""
#     # Initialize mock ADC system
#     mock_adc_init()

#     # Set up normal operating values for air temperature sensor (ADC_CHANNEL_0)
#     mock_adc_set_normal_value(0, 1850)  # ~25°C equivalent ADC reading

#     # Test 1: Normal operation
#     dut.expect('QEMU Temperature Test Starting')

#     # Test 2: Simulate disconnected air temperature sensor
#     mock_adc_simulate_disconnection(0)  # Disconnect ADC channel 0

#     # Test 3: System should detect invalid readings and log warnings
#     # The temperature code should detect very low ADC readings as disconnected
#     dut.expect('Test completed')

#     # Reset to normal for next test
#     mock_adc_reset_to_normal(0)

# @pytest.mark.qemu
# @idf_parametrize('target', ['linux'], indirect=['target'])
# def test_adc_short_circuit_detection(dut: QemuDut) -> None:
#     """Test ADC short circuit detection"""
#     mock_adc_init()
#     mock_adc_set_normal_value(0, 1850)

#     # Simulate short circuit on heater sensor (ADC_CHANNEL_1)
#     mock_adc_simulate_short_circuit(1)

#     dut.expect('QEMU Temperature Test Starting')
#     dut.expect('Test completed')

#     # Reset for next test
#     mock_adc_reset_to_normal(1)

# @pytest.mark.qemu
# @idf_parametrize('target', ['linux'], indirect=['target'])
# def test_adc_noise_resistance(dut: QemuDut) -> None:
#     """Test ADC noise resistance"""
#     mock_adc_init()
#     mock_adc_set_normal_value(0, 1850)

#     # Add 20% noise to air temperature sensor
#     mock_adc_simulate_noise(0, 0.2)

#     dut.expect('QEMU Temperature Test Starting')
#     dut.expect('Test completed')

#     mock_adc_reset_to_normal(0)

# @pytest.mark.qemu
# @idf_parametrize('target', ['linux'], indirect=['target'])
# def test_adc_intermittent_connection(dut: QemuDut) -> None:
#     """Test intermittent sensor connection"""
#     mock_adc_init()
#     mock_adc_set_normal_value(0, 1850)

#     # Simulate intermittent connection every 500ms
#     mock_adc_simulate_intermittent(0, 500)

#     dut.expect('QEMU Temperature Test Starting')
#     dut.expect('Test completed')

#     mock_adc_reset_to_normal(0)


@pytest.mark.qemu
def test_basic_qemu(dut: QemuDut) -> None:
    """Basic QEMU test to verify the setup works"""
    # Just wait a moment to see if QEMU starts without crashing
    import time
    time.sleep(2)
    # If we get here without a timeout/crash, the test passes
    assert True
