pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                sh "meson build"
                sh "ninja -C build"
            }
        }

       stage('Test') {
            steps {
                sh "ninja -C build test"
            }
        }
    }
}
