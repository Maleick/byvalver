#!/usr/bin/env python3
"""
Comprehensive test script to run byvalver on all .bin files in BIG_BIN directory
and collect detailed statistics and results for both biphasic and non-biphasic modes.
"""

import subprocess
import os
import json
import time
from pathlib import Path
from datetime import datetime
import tempfile

# Configuration
BIG_BIN_DIR = os.path.expanduser("~/RUBBISH/BIG_BIN")
BYVALVER_BIN = "./bin/byvalver"
OUTPUT_DIR = "./test_results"
RESULTS_FILE = f"{OUTPUT_DIR}/byvalver_bins_assessment_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
SUMMARY_FILE = f"{OUTPUT_DIR}/bins_summary_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"

def ensure_output_dir():
    """Create output directory if it doesn't exist."""
    Path(OUTPUT_DIR).mkdir(parents=True, exist_ok=True)

def get_bin_files():
    """Get all .bin files from BIG_BIN directory."""
    bin_files = []
    for file in sorted(os.listdir(BIG_BIN_DIR)):
        if file.endswith('.bin'):
            full_path = os.path.join(BIG_BIN_DIR, file)
            size = os.path.getsize(full_path)
            bin_files.append({
                'name': file,
                'path': full_path,
                'size': size
            })
    return bin_files

def analyze_shellcode_for_nulls(shellcode_data):
    """Analyze shellcode data to count null bytes."""
    null_count = 0
    for byte in shellcode_data:
        if byte == 0:
            null_count += 1
    return null_count

def run_byvalver(input_path, output_path, args=None, timeout=120):
    """Run byvalver on a single binary and capture results."""
    result = {
        'success': False,
        'output': '',
        'error': '',
        'exit_code': None,
        'execution_time': 0,
        'timed_out': False,
        'output_data': None,
        'nulls_remaining': -1,  # -1 means not determined yet
        'nulls_in_original': -1
    }

    start_time = time.time()

    # Prepare command
    cmd = [BYVALVER_BIN]
    if args:
        cmd.extend(args)
    # Use only input path and explicitly specify output with -o flag to avoid confusion
    cmd.extend([input_path, "-o", output_path])

    try:
        process = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout
        )

        result['exit_code'] = process.returncode
        result['output'] = process.stdout
        result['error'] = process.stderr

        # Check if process exited successfully
        if process.returncode == 0:
            # Read the output file to check for nulls
            if os.path.exists(output_path):
                with open(output_path, 'rb') as f:
                    result['output_data'] = f.read()
                    result['nulls_remaining'] = analyze_shellcode_for_nulls(result['output_data'])

                # Also analyze original file for comparison
                with open(input_path, 'rb') as f:
                    original_data = f.read()
                    result['nulls_in_original'] = analyze_shellcode_for_nulls(original_data)

                # Determine true success - process must exit 0 AND eliminate all nulls
                result['success'] = (process.returncode == 0 and result['nulls_remaining'] == 0)
            else:
                # Output file wasn't created, so it's not successful
                result['success'] = False
        else:
            result['success'] = False

    except subprocess.TimeoutExpired:
        result['timed_out'] = True
        result['error'] = f"Process timed out after {timeout} seconds"
    except Exception as e:
        result['error'] = str(e)
        result['success'] = False

    result['execution_time'] = time.time() - start_time
    return result

def parse_byvalver_output(output, error):
    """Parse byvalver output to extract statistics."""
    stats = {
        'original_size': 0,
        'modified_size': 0,
        'instructions_disassembled': 0,
        'null_bytes_found': 0,
        'strategies_applied': [],
        'warnings': [],
        'has_nulls_eliminated': False
    }

    # Parse output
    for line in output.split('\n'):
        if 'Original shellcode size:' in line:
            stats['original_size'] = int(line.split(':')[1].strip())
        elif 'Modified shellcode size:' in line:
            stats['modified_size'] = int(line.split(':')[1].strip())

    # Parse error/debug output
    for line in error.split('\n'):
        if '[DISASM] Disassembled' in line:
            parts = line.split()
            if len(parts) >= 3:
                stats['instructions_disassembled'] = int(parts[2])

        if '[TRACE] Using strategy' in line:
            if "'" in line:
                strategy_name = line.split("'")[1]
                if strategy_name not in stats['strategies_applied']:
                    stats['strategies_applied'].append(strategy_name)

        if '[WARNING]' in line:
            warning = line.split('[WARNING]')[1].strip()[:100]
            if warning not in stats['warnings']:
                stats['warnings'].append(warning)

        if 'has_null=1' in line:
            stats['null_bytes_found'] += 1

        if 'eliminated' in line.lower() or 'transformed' in line.lower():
            stats['has_nulls_eliminated'] = True

    return stats

def main():
    """Main test execution."""
    print("=" * 80)
    print("BYVALVER .BIN FILES COMPREHENSIVE ASSESSMENT")
    print("Testing both with and without --biphasic flag")
    print("=" * 80)
    print()

    ensure_output_dir()

    # Get all binaries
    bin_files = get_bin_files()
    total_count = len(bin_files)
    total_size = sum(f['size'] for f in bin_files)

    print(f"Found {total_count} .bin files")
    print(f"Total size: {total_size / (1024*1024):.2f} MB")
    print(f"Output will be saved to: {RESULTS_FILE}")
    print()

    # Test results storage
    results = {
        'metadata': {
            'test_date': datetime.now().isoformat(),
            'total_files': total_count,
            'total_size_bytes': total_size,
            'byvalver_path': BYVALVER_BIN,
            'source_directory': BIG_BIN_DIR
        },
        'tests': []
    }

    # Statistics
    stats = {
        'total': total_count,
        'successful_biphasic': 0,
        'failed_biphasic': 0,
        'timed_out_biphasic': 0,
        'errors_biphasic': 0,
        'successful_non_biphasic': 0,
        'failed_non_biphasic': 0,
        'timed_out_non_biphasic': 0,
        'errors_non_biphasic': 0,
        'total_instructions_biphasic': 0,
        'total_instructions_non_biphasic': 0,
        'total_null_bytes_found_biphasic': 0,
        'total_null_bytes_found_non_biphasic': 0,
        'files_with_nulls_biphasic': 0,
        'files_with_nulls_non_biphasic': 0,
        'strategies_used_biphasic': {},
        'strategies_used_non_biphasic': {}
    }

    # Run tests
    print("Starting tests...")
    print("-" * 80)

    for idx, bin_file in enumerate(bin_files, 1):
        name = bin_file['name']
        path = bin_file['path']
        size = bin_file['size']

        print(f"[{idx}/{total_count}] Testing: {name} ({size/1024:.1f} KB)")
        
        file_result = {
            'index': idx,
            'filename': name,
            'path': path,
            'size_bytes': size,
            'biphasic_test': None,
            'non_biphasic_test': None
        }

        # Test with biphasic mode
        with tempfile.NamedTemporaryFile(delete=False, suffix='.bin') as biphasic_out:
            biphasic_out_path = biphasic_out.name

        print(f"  Testing biphasic mode...", end=' ', flush=True)
        biphasic_result = run_byvalver(path, biphasic_out_path, ['--biphasic'])
        
        # Parse output for statistics
        biphasic_parsed_stats = parse_byvalver_output(biphasic_result['output'], biphasic_result['error'])
        biphasic_result['parsed_stats'] = biphasic_parsed_stats

        # Determine status for biphasic test
        if biphasic_result['timed_out']:
            biphasic_status = "TIMEOUT"
            stats['timed_out_biphasic'] += 1
        elif biphasic_result['success']:
            biphasic_status = "SUCCESS"  # Process completed AND nulls eliminated
            stats['successful_biphasic'] += 1
        elif biphasic_result['exit_code'] == 0 and biphasic_result['nulls_remaining'] > 0:
            biphasic_status = "NULLS_REMAINING"  # Process completed but nulls remain
            stats['failed_biphasic'] += 1
        elif biphasic_result['exit_code'] != 0:
            biphasic_status = "ERROR"
            stats['errors_biphasic'] += 1
        else:
            biphasic_status = "FAILED"
            stats['failed_biphasic'] += 1

        biphasic_time = biphasic_result['execution_time']
        biphasic_nulls = biphasic_result['nulls_remaining']
        biphasic_original_nulls = biphasic_result['nulls_in_original']
        biphasic_instructions = biphasic_parsed_stats['instructions_disassembled']

        print(f"{biphasic_status} ({biphasic_time:.3f}s, {biphasic_instructions} insns, {biphasic_original_nulls}->{biphasic_nulls} nulls)")

        # Update biphasic stats
        stats['total_instructions_biphasic'] += biphasic_instructions
        stats['total_null_bytes_found_biphasic'] += biphasic_nulls

        if biphasic_nulls > 0:
            stats['files_with_nulls_biphasic'] += 1

        for strategy in biphasic_parsed_stats['strategies_applied']:
            stats['strategies_used_biphasic'][strategy] = stats['strategies_used_biphasic'].get(strategy, 0) + 1

        # Clean up biphasic output file
        try:
            os.unlink(biphasic_out_path)
        except:
            pass

        # Test without biphasic mode
        with tempfile.NamedTemporaryFile(delete=False, suffix='.bin') as non_biphasic_out:
            non_biphasic_out_path = non_biphasic_out.name

        print(f"  Testing non-biphasic mode...", end=' ', flush=True)
        non_biphasic_result = run_byvalver(path, non_biphasic_out_path)
        
        # Parse output for statistics
        non_biphasic_parsed_stats = parse_byvalver_output(non_biphasic_result['output'], non_biphasic_result['error'])
        non_biphasic_result['parsed_stats'] = non_biphasic_parsed_stats

        # Determine status for non-biphasic test
        if non_biphasic_result['timed_out']:
            non_biphasic_status = "TIMEOUT"
            stats['timed_out_non_biphasic'] += 1
        elif non_biphasic_result['success']:
            non_biphasic_status = "SUCCESS"  # Process completed AND nulls eliminated
            stats['successful_non_biphasic'] += 1
        elif non_biphasic_result['exit_code'] == 0 and non_biphasic_result['nulls_remaining'] > 0:
            non_biphasic_status = "NULLS_REMAINING"  # Process completed but nulls remain
            stats['failed_non_biphasic'] += 1
        elif non_biphasic_result['exit_code'] != 0:
            non_biphasic_status = "ERROR"
            stats['errors_non_biphasic'] += 1
        else:
            non_biphasic_status = "FAILED"
            stats['failed_non_biphasic'] += 1

        non_biphasic_time = non_biphasic_result['execution_time']
        non_biphasic_nulls = non_biphasic_result['nulls_remaining']
        non_biphasic_original_nulls = non_biphasic_result['nulls_in_original']
        non_biphasic_instructions = non_biphasic_parsed_stats['instructions_disassembled']

        print(f"  {non_biphasic_status} ({non_biphasic_time:.3f}s, {non_biphasic_instructions} insns, {non_biphasic_original_nulls}->{non_biphasic_nulls} nulls)")

        # Update non-biphasic stats
        stats['total_instructions_non_biphasic'] += non_biphasic_instructions
        stats['total_null_bytes_found_non_biphasic'] += non_biphasic_nulls

        if non_biphasic_nulls > 0:
            stats['files_with_nulls_non_biphasic'] += 1

        for strategy in non_biphasic_parsed_stats['strategies_applied']:
            stats['strategies_used_non_biphasic'][strategy] = stats['strategies_used_non_biphasic'].get(strategy, 0) + 1

        # Clean up non-biphasic output file
        try:
            os.unlink(non_biphasic_out_path)
        except:
            pass

        # Store detailed results
        file_result['biphasic_test'] = {
            'status': biphasic_status,
            'execution_time': biphasic_time,
            'exit_code': biphasic_result['exit_code'],
            'timed_out': biphasic_result['timed_out'],
            'output': biphasic_result['output'],
            'error': biphasic_result['error'][:2000] if biphasic_result['error'] else '',
            'nulls_remaining': biphasic_nulls,
            'nulls_in_original': biphasic_original_nulls,
            'parsed_stats': biphasic_parsed_stats
        }

        file_result['non_biphasic_test'] = {
            'status': non_biphasic_status,
            'execution_time': non_biphasic_time,
            'exit_code': non_biphasic_result['exit_code'],
            'timed_out': non_biphasic_result['timed_out'],
            'output': non_biphasic_result['output'],
            'error': non_biphasic_result['error'][:2000] if non_biphasic_result['error'] else '',
            'nulls_remaining': non_biphasic_nulls,
            'nulls_in_original': non_biphasic_original_nulls,
            'parsed_stats': non_biphasic_parsed_stats
        }

        results['tests'].append(file_result)

    # Save results
    print()
    print("-" * 80)
    print("Saving results...")

    results['statistics'] = stats

    with open(RESULTS_FILE, 'w') as f:
        json.dump(results, f, indent=2)

    # Generate summary report
    summary = []
    summary.append("=" * 80)
    summary.append("BYVALVER .BIN FILES ASSESSMENT SUMMARY")
    summary.append("Testing both with and without --biphasic flag")
    summary.append("=" * 80)
    summary.append("")
    summary.append(f"Test Date: {results['metadata']['test_date']}")
    summary.append(f"Total Files Tested: {total_count}")
    summary.append(f"Total Size: {total_size / (1024*1024):.2f} MB")
    summary.append("")
    summary.append("BIPHASIC MODE RESULTS:")
    summary.append(f"  Successful (all nulls eliminated): {stats['successful_biphasic']} ({stats['successful_biphasic']/total_count*100:.1f}%)")
    summary.append(f"  Failed (nulls remain):             {stats['failed_biphasic']} ({stats['failed_biphasic']/total_count*100:.1f}%)")
    summary.append(f"  Errors:                            {stats['errors_biphasic']} ({stats['errors_biphasic']/total_count*100:.1f}%)")
    summary.append(f"  Timeouts:                          {stats['timed_out_biphasic']} ({stats['timed_out_biphasic']/total_count*100:.1f}%)")
    summary.append("")
    summary.append("NON-BIPHASIC MODE RESULTS:")
    summary.append(f"  Successful (all nulls eliminated): {stats['successful_non_biphasic']} ({stats['successful_non_biphasic']/total_count*100:.1f}%)")
    summary.append(f"  Failed (nulls remain):             {stats['failed_non_biphasic']} ({stats['failed_non_biphasic']/total_count*100:.1f}%)")
    summary.append(f"  Errors:                            {stats['errors_non_biphasic']} ({stats['errors_non_biphasic']/total_count*100:.1f}%)")
    summary.append(f"  Timeouts:                          {stats['timed_out_non_biphasic']} ({stats['timed_out_non_biphasic']/total_count*100:.1f}%)")
    summary.append("")

    # Calculate averages
    avg_biphasic_time = sum(t['biphasic_test']['execution_time'] for t in results['tests']) / total_count
    avg_non_biphasic_time = sum(t['non_biphasic_test']['execution_time'] for t in results['tests']) / total_count

    summary.append(f"Average Execution Time (Biphasic):     {avg_biphasic_time:.3f} seconds")
    summary.append(f"Average Execution Time (Non-biphasic): {avg_non_biphasic_time:.3f} seconds")
    summary.append("")

    # Null byte statistics
    summary.append("NULL BYTE ANALYSIS:")
    summary.append(f"  Total null bytes found (biphasic):   {stats['total_null_bytes_found_biphasic']}")
    summary.append(f"  Files with nulls remaining (biphasic): {stats['files_with_nulls_biphasic']} ({stats['files_with_nulls_biphasic']/total_count*100:.1f}%)")
    summary.append(f"  Total null bytes found (non-biphasic): {stats['total_null_bytes_found_non_biphasic']}")
    summary.append(f"  Files with nulls remaining (non-biphasic): {stats['files_with_nulls_non_biphasic']} ({stats['files_with_nulls_non_biphasic']/total_count*100:.1f}%)")
    summary.append("")

    # Strategy usage
    summary.append("STRATEGIES APPLIED (BIPHASIC):")
    for strategy, count in sorted(stats['strategies_used_biphasic'].items(), key=lambda x: -x[1])[:20]:  # Top 20
        summary.append(f"  {strategy}: {count} times")
    summary.append("")

    summary.append("STRATEGIES APPLIED (NON-BIPHASIC):")
    for strategy, count in sorted(stats['strategies_used_non_biphasic'].items(), key=lambda x: -x[1])[:20]:  # Top 20
        summary.append(f"  {strategy}: {count} times")
    summary.append("")

    # Compare modes
    summary.append("MODE COMPARISON:")
    better_biphasic = 0
    better_non_biphasic = 0
    same_result = 0
    for test in results['tests']:
        biphasic_success = test['biphasic_test']['status'] in ['SUCCESS']
        non_biphasic_success = test['non_biphasic_test']['status'] in ['SUCCESS']
        
        if biphasic_success and not non_biphasic_success:
            better_biphasic += 1
        elif non_biphasic_success and not biphasic_success:
            better_non_biphasic += 1
        else:
            same_result += 1
    
    summary.append(f"  Biphase better:  {better_biphasic} files ({better_biphasic/total_count*100:.1f}%)")
    summary.append(f"  Non-biphase better: {better_non_biphasic} files ({better_non_biphasic/total_count*100:.1f}%)")
    summary.append(f"  Same result:     {same_result} files ({same_result/total_count*100:.1f}%)")
    summary.append("")

    # List files where biphasic mode was better
    if better_biphasic > 0:
        summary.append("FILES WHERE BIPHASIC MODE WAS BETTER:")
        for test in results['tests']:
            biphasic_success = test['biphasic_test']['status'] in ['SUCCESS']
            non_biphasic_success = test['non_biphasic_test']['status'] in ['SUCCESS']
            if biphasic_success and not non_biphasic_success:
                summary.append(f"  {test['filename']}: {test['biphasic_test']['status']} vs {test['non_biphasic_test']['status']}")
        summary.append("")

    # List files where non-biphasic mode was better
    if better_non_biphasic > 0:
        summary.append("FILES WHERE NON-BIPHASIC MODE WAS BETTER:")
        for test in results['tests']:
            biphasic_success = test['biphasic_test']['status'] in ['SUCCESS']
            non_biphasic_success = test['non_biphasic_test']['status'] in ['SUCCESS']
            if non_biphasic_success and not biphasic_success:
                summary.append(f"  {test['filename']}: {test['non_biphasic_test']['status']} vs {test['biphasic_test']['status']}")
        summary.append("")

    summary.append("=" * 80)
    summary.append(f"Detailed results saved to: {RESULTS_FILE}")
    summary.append("=" * 80)

    summary_text = "\n".join(summary)

    # Save summary
    with open(SUMMARY_FILE, 'w') as f:
        f.write(summary_text)

    # Print summary
    print()
    print(summary_text)

if __name__ == "__main__":
    main()