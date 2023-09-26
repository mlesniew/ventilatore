import gzip
import shutil
import os
import hashlib

Import("env")


def file_hash(path):
    if not os.path.isfile(path):
        return None
    with open(path, "rb") as f:
        return hashlib.sha1(f.read()).hexdigest()


def compress_for_ota(source, target, env):
    firmware_path = str(source[0])
    uncompressed_firmware_path = firmware_path + ".uncompressed"
    compressed_firmware_path = firmware_path + ".compressed"

    firmware_hash = file_hash(firmware_path)
    uncompressed_firmware_installed = firmware_hash == file_hash(uncompressed_firmware_path)
    compressed_firmware_installed = firmware_hash == file_hash(compressed_firmware_path)

    if not uncompressed_firmware_installed and not compressed_firmware_installed:
        os.unlink(compressed_firmware_path) if os.path.exists(compressed_firmware_path) else None
        shutil.move(firmware_path, uncompressed_firmware_path)

    uploader = env["UPLOADER"]
    compressed_image_needed = os.path.basename(uploader) == "espota.py"

    if compressed_image_needed:
        print("Using compressed binary for upload")
        if not compressed_firmware_installed:
            print(f"Creating {compressed_firmware_path}")
            with open(uncompressed_firmware_path, "rb") as input_file, gzip.open(compressed_firmware_path, "wb", 9) as output_file:
                shutil.copyfileobj(input_file, output_file)

            shutil.copy(compressed_firmware_path, firmware_path)

        original_size = os.path.getsize(uncompressed_firmware_path)
        compressed_size = os.path.getsize(compressed_firmware_path)
        ratio = compressed_size / original_size
        print(f"Compressed binary size is {compressed_size} bytes, compression ratio: {ratio * 100:.1f}%")
    else:
        print("Using uncompressed binary for upload")
        if not uncompressed_firmware_installed:
            shutil.copy(uncompressed_firmware_path, firmware_path)

env.AddPreAction("upload", compress_for_ota)
