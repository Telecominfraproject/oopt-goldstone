import setuptools

with open("requirements.txt", "r") as f:
    install_requires = f.read().split()

setuptools.setup(
        name='galileo',
        version='0.2.0',
        install_requires=install_requires,
        description='Galileo Debug Tool',
        python_requires='>=3.7',
        entry_points={
            'console_scripts': [
                'galileo = galileo.main:main',
            ],
        },
        packages=setuptools.find_packages(),
        zip_safe = False,
)
