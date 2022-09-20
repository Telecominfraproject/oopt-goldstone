pipeline {
  agent any

  parameters {
    string(name: 'GFT_DEVICE', defaultValue: '192.168.201.201', description: 'IP address of the GFT test device')
    string(name: 'GALILEO1_DEVICE', defaultValue: '192.168.201.202', description: 'IP address of the G1 test device')
    booleanParam(name: 'CLEAN', defaultValue: false, description: 'clean before building')
    booleanParam(name: 'SKIP_ROBOT', defaultValue: true, description: 'skip ROBOT framework testing')
    booleanParam(name: 'SKIP_REBUILD_MGMT', defaultValue: true, description: 'skip rebuilding mgmt images')
  }

  stages {
    stage('Setup') {
      steps {
          sh 'env'
          script {
              env.SKIP = 0
              env.SKIP_ROBOT = params.SKIP_ROBOT
              env.SKIP_REBUILD_MGMT = params.SKIP_REBUILD_MGMT
              if (env.BRANCH_NAME == 'master' ) {
                  env.BUILD_KERNEL = 1
              } else if ( env.BRANCH_NAME.startsWith('PR') ) {
                  env.BUILD_KERNEL = sh returnStatus: true, script: "git diff --compact-summary HEAD origin/master | grep -v 'sm/ONL'"
              } else {
                  env.SKIP = 1
                  env.BUILD_KERNEL = 0
                  currentBuild.result = 'SUCCESS'
                  echo "no need to build ${env.BRANCH_NAME}"
              }
          }
          sh 'rm -rf RELEASE make/versions'
          sh 'env'
      }
    }

    stage('Clean') {
      when {
        expression { params.CLEAN }
      }
      steps {
        sh 'git clean -dfx'
        sh 'find REPO'
      }
    }

    stage('Build') {
      when {
        environment name: 'SKIP', value: '0'
      }
      steps {
        sh 'apk add --update docker make python2 python3 make'
        // sh 'git fetch --tags' configure in Jenkins Web GUI. check "Advanced Clone Behaviours"
        sh 'git submodule update --init'

        sh 'make builder'
        sh 'make docker'

        sh """
            ONIE_IMAGE=\$(find RELEASE -regex ".*AMD64_INSTALLER\$" -exec ls -t1 "{}" + | head -n 1)
            cp \${ONIE_IMAGE} /var/nginx_home/onie-installer-x86_64-ci
            ONIE_IMAGE=\$(find RELEASE -regex ".*ARM64_INSTALLER\$" -exec ls -t1 "{}" + | head -n 1)
            cp \${ONIE_IMAGE} /var/nginx_home/onie-installer-arm64-ci
        """
      }
    }
    stage('Run Ansible ZTP script') {
      when {
        branch pattern: "^PR.*", comparator: "REGEXP"
        environment name: 'SKIP', value: '0'
      }
      steps {
        dir("ci/ansible") {
          sh 'make image'
          sh 'DOCKER_RUN_OPTION="-t" DOCKER_CMD="make play" make cmd'
        }
      }
    }
  }

  post {
    success {
      script {
        if ( env.BRANCH_NAME == 'master' ) {
            sh """
                ONIE_IMAGE=\$(find RELEASE -regex ".*AMD64_INSTALLER\$" -exec ls -t1 "{}" + | head -n 1)
                cp \${ONIE_IMAGE} /var/nginx_home/
                cd /var/nginx_home && ln -sf \$(basename \${ONIE_IMAGE}) onie-installer-x86_64
            """

            sh """
                ONIE_IMAGE=\$(find RELEASE -regex ".*ARM64_INSTALLER\$" -exec ls -t1 "{}" + | head -n 1)
                cp \${ONIE_IMAGE} /var/nginx_home/
                cd /var/nginx_home && ln -sf \$(basename \${ONIE_IMAGE}) onie-installer-arm64
            """

            sh "rm -rf /var/libraries/REPO"
            sh "cp -r REPO /var/libraries/"
            archiveArtifacts artifacts: 'RELEASE/**/*_INSTALLER', fingerprint: true
        }
      }
    }
    cleanup {
      deleteDir() /* clean up our workspace */
      sh 'rm -f ~/.gitconfig'
    }
  }

}
// vim: ft=groovy
