#!/usr/bin/env python3
"""
Manages firmware release versioning for PocketMage web flasher.
Handles preservation of releases (kept forever) and dev builds (keep last N).
"""

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple


class FirmwareReleaseManager:
    def __init__(self, workspace: Path, keep_dev_count: int = 2):
        self.workspace = workspace
        self.docs_dir = workspace / "docs"
        self.firmware_dir = self.docs_dir / "firmware"
        self.keep_dev_count = keep_dev_count
        
    def clone_gh_pages(self, repo_url: str, target_dir: Path) -> bool:
        """Clone the gh-pages branch to inspect existing releases."""
        try:
            subprocess.run(
                ["git", "clone", "--depth=1", "--branch", "gh-pages", repo_url, str(target_dir)],
                check=True,
                capture_output=True,
                text=True
            )
            return True
        except subprocess.CalledProcessError:
            return False
    
    def get_version_dirs(self, base_dir: Path, pattern: str = "*") -> List[str]:
        """Get all version directories matching the pattern, sorted."""
        if not base_dir.exists():
            return []
        
        dirs = [d.name for d in base_dir.iterdir() if d.is_dir() and d.match(pattern)]
        return sorted(dirs)
    
    def is_dev_version(self, version: str) -> bool:
        """Check if a version string is a dev build."""
        return version.startswith("dev-")
    
    def preserve_from_gh_pages(self, gh_pages_dir: Path, current_version: str):
        """Copy release and dev builds from gh-pages to workspace."""
        gh_firmware = gh_pages_dir / "firmware"
        
        if not gh_firmware.exists():
            print("No firmware directory in gh-pages yet")
            return
        
        release_dirs = self.get_version_dirs(gh_firmware, "*")
        release_dirs = [d for d in release_dirs if not self.is_dev_version(d)]
        
        for release in release_dirs:
            if release == current_version:
                print(f"  Skipping preservation of {release} (current build)")
                continue
                
            print(f"  Preserving release: {release}")
            src = gh_firmware / release
            dst = self.firmware_dir / release
            if dst.exists():
                shutil.rmtree(dst)
            shutil.copytree(src, dst)
        
        dev_dirs = self.get_version_dirs(gh_firmware, "dev-*")
        
        if dev_dirs:
            if self.is_dev_version(current_version):
                keep_count = self.keep_dev_count - 1
                print(f"  Current build is dev, keeping {keep_count} previous dev builds")
            else:
                keep_count = self.keep_dev_count
                print(f"  Current build is release, keeping {keep_count} dev builds")
            
            dev_dirs_to_keep = dev_dirs[-keep_count:] if keep_count > 0 else []
            
            for dev_dir in dev_dirs_to_keep: 
                if dev_dir != current_version: 
                    print(f"  Preserving dev build: {dev_dir}")
                    src = gh_firmware / dev_dir
                    dst = self.firmware_dir / dev_dir
                    if dst.exists():
                        shutil.rmtree(dst)
                    shutil.copytree(src, dst)
    
    def _safe_version_sort_key(self, version_str: str):
        """
        Create a sort key for version strings.
        Tries to parse as PEP 440 version first, falls back to string comparison.
        Returns a tuple (is_valid_pep440, sort_key) to group valid versions first.
        """
        try:
            from packaging import version
            return (1, version.parse(version_str))
        except Exception:
            return (0, version_str.lower())
    
    def rebuild_manifest_index(self):
        """Rebuild manifest-index.json from actual firmware directories."""
        if not self.firmware_dir.exists():
            versions = []
        else:
            all_dirs = self.get_version_dirs(self.firmware_dir)
            
            dev_versions = [v for v in all_dirs if self.is_dev_version(v)]
            release_versions = [v for v in all_dirs if not self.is_dev_version(v)]
            
            dev_versions.sort(reverse=True)
            
            release_versions.sort(key=self._safe_version_sort_key, reverse=True)
            
            versions = dev_versions + release_versions
        
        manifest_index = {"versions": versions}
        manifest_path = self.docs_dir / "manifest-index.json"
        
        with open(manifest_path, "w") as f:
            json.dump(manifest_index, f, indent=2)
        
        print(f"\nManifest index updated with {len(versions)} versions")
        return manifest_index
    
    def run(self, repo_url: str, current_version: str, gh_pages_dir: Path):
        """Main execution flow."""
        print("Preserving existing releases and dev builds from gh-pages...")
        
        self.firmware_dir.mkdir(parents=True, exist_ok=True)
        
        if self.clone_gh_pages(repo_url, gh_pages_dir):
            self.preserve_from_gh_pages(gh_pages_dir, current_version)
        else:
            print("No existing gh-pages branch found")
        
        self.rebuild_manifest_index()
        
        print("\n✓ Firmware release management complete")


def main():
    parser = argparse.ArgumentParser(description="Manage PocketMage firmware releases")
    parser.add_argument("--workspace", required=True, help="GitHub workspace directory")
    parser.add_argument("--version", required=True, help="Current version being built")
    parser.add_argument("--repo-url", required=True, help="Repository URL with token")
    parser.add_argument("--gh-pages-dir", required=True, help="Temp directory for gh-pages clone")
    parser.add_argument("--keep-dev", type=int, default=2, help="Number of dev builds to keep")
    
    args = parser.parse_args()
    
    manager = FirmwareReleaseManager(
        workspace=Path(args.workspace),
        keep_dev_count=args.keep_dev
    )
    
    manager.run(
        repo_url=args.repo_url,
        current_version=args.version,
        gh_pages_dir=Path(args.gh_pages_dir)
    )


if __name__ == "__main__": 
    main()
