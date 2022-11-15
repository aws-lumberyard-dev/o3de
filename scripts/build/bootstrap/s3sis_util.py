#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import argparse
import boto3
import github
import json
import subprocess
from s3sis.s3sislib.s3client import S3Client
from s3sis.s3sislib.manifest import Manifest
from datetime import datetime, timedelta
from botocore.exceptions import ClientError
 

SECRET_NAME = 'jenkins-github/service-accounts/o3de-issues-bot'

"""
/usr/local/bin/python3 /Users/shiranj/workspace/github/o3de/scripts/build/Jenkins/tools/s3sis_util.py --action download --repository aws-lumberyard-dev/o3de --branch s3sis_build --pipeline O3DE-LY-FORK --platform Windows --build-config profile --workspace /Users/shiranj/workspace/github/o3de/scripts/build/Platform
"""



def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--action', dest='action', required=True, help='Repository name')
    parser.add_argument('--repository', dest='repository', required=True, help='Repository name')
    parser.add_argument('--branch', dest='branch', required=True, help='Branch name')
    parser.add_argument('--pipeline', dest='pipeline', required=True, help='Pipeline name')
    parser.add_argument('--platform', dest='platform', required=True, help='Platform')
    parser.add_argument('--build-config', dest='build_config', required=True, help='Build config')
    parser.add_argument('--workspace', dest='workspace', required=True, help='Workspace path')
    parser.add_argument('--include', dest='include', default=None, required=False, help='Files to include')
    parser.add_argument('--exclude', dest='exclude', default=None, required=False, help='Files to exclude')
    args = parser.parse_args()
    return args


def get_secret(secret_name):
    secrets = boto3.client(service_name='secretsmanager', region_name='us-west-2')
    try:
        response = secrets.get_secret_value(SecretId=secret_name)
        return json.loads(response['SecretString'])
    except ClientError as e:
        print(f'ERROR: Unable to get secret value: {e}')
        exit(1)


def is_exists_on_s3(label):
    s3_client = S3Client()
    manifest = Manifest(workspace=args.workspace, label=label)
    manifest_s3_key = s3_client.manifest_key(manifest)
    return s3_client.object_exists_on_s3(manifest_s3_key)


def get_commits(repository, branch, since):
    secret = get_secret(SECRET_NAME)
    github_client = github.Github(secret['Github Token'])
    if branch.startswith('PR-'):
        repo = github_client.get_repo(repository)
        pr_number = int(branch[3:])
        pr = repo.get_pull(pr_number)
        branch = pr.head.ref
        repository = (pr.head.repo.full_name)
    repo = github_client.get_repo(repository)
    commits = repo.get_commits(sha=branch, since=since)
    commits = [commit.sha for commit in commits]
    return commits


def replace_slash(s):
    return s.replace('/', '-')


def create_labels(args, commits):
    labels = []
    for commit in commits:
        label = f'{replace_slash(args.pipeline)}/{replace_slash(args.platform)}/{replace_slash(args.build_config)}/{commit}'
        labels.append(label)
    return labels
    

def upload(args):
    all_commits = get_commits(args.repository, args.branch, datetime.today() - timedelta(days=30))
    # Exclude new commits submitted after current build start time.
    current_commit = os.environ.get('CHANGE_ID')
    commits = []
    for idx, commit in enumerate(all_commits):
        if commit == current_commit:
            commits = all_commits[idx:]
            break
    labels = create_labels(args, commits)
    labels_to_upload = []
    for label in labels:
        if not is_exists_on_s3(label):
            labels_to_upload.append(label)
        else:
            break
    for idx, label_to_upload in enumerate(labels_to_upload):
        if idx != 0:
            cmd = ['s3siscli', 'manifest', '--label', label_to_upload, '--update']
            print(cmd)
            process_return = subprocess.run(cmd)
        cmd = ['s3siscli', 'upload', '--label', label_to_upload, '--workspace' ,args.workspace]
        if args.include:
            cmd += ['--include', args.include]
        if args.exclude:
            cmd += ['--exclude', args.exclude]
        if idx != 0:
            cmd += ['--no-regenerate-manifest']
        print(cmd)
        process_return = subprocess.run(cmd)
    

def download(args):
    commits = get_commits(args.repository, args.branch, datetime.today() - timedelta(days=2))
    print(commits)
    labels = create_labels(args, commits)
    label_to_download = None
    for label in labels:
        if is_exists_on_s3(label):
            label_to_download = label
            break
    cmd = ['s3siscli', 'download', '--label', label_to_download, '--workspace' ,args.workspace]
    if args.include:
        cmd += ['--include', args.include]
    if args.exclude:
        cmd += ['--exclude', args.exclude]
    print(cmd)
    process_return = subprocess.run(cmd)


if __name__ == "__main__":
    args = parse_args()
    if args.action == 'upload':
        upload(args)
    elif args.action == 'download':
        download(args)



