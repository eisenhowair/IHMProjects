pipeline {
    agent any
    tools {
        msbuild 'MSBuild' // Configure MSBuild dans Jenkins
    }
    environment {
        BUILD_CONFIG = 'Release' // Mode de compilation
    }
    stages {
        stage('Checkout') {
            steps {
                git branch: 'main', url: 'https://github.com/eisenhowair/IHMProjects'
            }
        }
        stage('Find Solutions') {
            steps {
                script {
                    // Cherche tous les fichiers .sln dans l’arborescence du projet
                    env.SLN_FILES = sh(script: 'find . -name "*.sln"', returnStdout: true).trim()
                    if (!env.SLN_FILES) {
                        error "Aucun fichier solution trouvé dans le dépôt !"
                    }
                    echo "Solutions trouvées :\n${env.SLN_FILES}"
                }
            }
        }
        stage('Build Solutions') {
            steps {
                script {
                    // Parcourt chaque fichier solution trouvé
                    env.SLN_FILES.split("\n").each { sln ->
                        bat "\"C:\\Program Files\\Microsoft Visual Studio\\BuildTools\\MSBuild\\Current\\Bin\\MSBuild.exe\" ${sln} /p:Configuration=${env.BUILD_CONFIG}"
                    }
                }
            }
        }
        stage('Run Tests') {
            steps {
                script {
                    // Exécute les tests unitaires pour chaque solution (si applicable)
                    env.SLN_FILES.split("\n").each { sln ->
                        def testDllPath = sln.replace('.sln', "/x64/${env.BUILD_CONFIG}/*.dll")
                        bat "\"C:\\Program Files\\Microsoft Visual Studio\\TestPlatform\\vstest.console.exe\" ${testDllPath}"
                    }
                }
            }
        }
        stage('Archive Artifacts') {
            steps {
                archiveArtifacts artifacts: '**/x64/Release/**/*', fingerprint: true
            }
        }
    }
    post {
        always {
            echo 'Pipeline terminée, succès ou échec !'
        }
        success {
            echo 'Build et tests réussis avec succès !'
        }
        failure {
            echo 'Échec de la pipeline. Vérifie les logs pour plus de détails.'
        }
    }
}
