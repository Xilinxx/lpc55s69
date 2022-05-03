pipeline {
    agent {
        label "linux"
    }

    options {
        disableConcurrentBuilds()
    }

    stages {
        stage("Parallel stages") {
            parallel {
                stage("Code Analysis") {
                    agent {
                        label "linux"
                    }
                    steps {
                        script {
                            sh "./s300-scripts/test.sh -t cmocka --xml"
                            archiveArtifacts artifacts: "test-cmocka/bin/*.xml",
                                             fingerprint: true,
                                             onlyIfSuccessful: true
                            junit testResults: "test-cmocka/bin/testcase_tests_that_should_fail.xml",
                                  allowEmptyResults: false
                            junit testResults: "test-cmocka/bin/testcase_tests_that_should_pass.xml",
                                  allowEmptyResults: false
                            junit testResults: "test-cmocka/bin/testcase_bootcode_tests.xml",
                                  allowEmptyResults: false
                            junit testResults: "test-cmocka/bin/testcase_tests_array.xml",
                                  allowEmptyResults: false
                            junit testResults: "test-cmocka/bin/testcase_tests_protocol.xml",
                                  allowEmptyResults: false
                            junit testResults: "test-cmocka/bin/testcase_gowin_protocol_tests.xml",
                                  allowEmptyResults: false
                        }
                    }
                }

                stage("Flash tool") {
                    agent {
                        label "linux"
                    }
                    steps {
                        script {
                            sh "./s300-scripts/build.sh -t buildroot "
                            archiveArtifacts artifacts: "build-buildroot/bin/flash_tool",
                                             fingerprint: true,
                                             onlyIfSuccessful: true
                        }
                    }
                }

                stage("GPMCU") {
                    agent {
                        label "linux"
                    }
                    steps {
                        script {
                            sh "./s300-scripts/build.sh -t arm"
                            archiveArtifacts artifacts: "build-arm/bin/*",
                                             fingerprint: true,
                                             onlyIfSuccessful: true
                            archiveArtifacts artifacts: "build-arm/*.map",
                                             fingerprint: true,
                                             onlyIfSuccessful: true
                            archiveArtifacts artifacts: "build-arm/buildinfo.txt",
                                             fingerprint: true,
                                             onlyIfSuccessful: true
                            // upload to Artifactory
                            board = "zeus300s"
                            if (env.TAG_NAME == null) {
                                artifactory_repo = "p300-dev-local"
                                artifactory_target = "gpmcu/" + board + "/" + BRANCH_NAME + "/"
                                version = env.GIT_COMMIT.take(7)
                            } else {
                                artifactory_repo = "p300-local"
                                artifactory_target = "gpmcu/" + board + "/"
                                version = env.TAG_NAME
                            }
                            output_package = "gpmcu-" + board + "-" + version + ".tar.gz"
                            sh "tar -czvf ${output_package} --directory=build-arm/bin ."
                            rtUpload (
                                serverId: artifactory_repo,
                                spec: """{
                                    "files": [
                                        {
                                            "pattern": "${output_package}",
                                            "target": "${artifactory_target}"
                                        }
                                    ]
                                }"""
                            )
                        }
                    }
                }
            }
        }
    }
}
