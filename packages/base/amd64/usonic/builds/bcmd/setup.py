import setuptools

with open("requirements.txt", "r") as f:
    install_requires = f.read().split()

setuptools.setup(
    name="bcmd",
    version="0.1.0",
    install_requires=install_requires,
    entry_points={
        "console_scripts": [
            "bcmd = bcmd.server:main",
            "bcmsh = bcmd.client:main",
        ],
    },
    packages=["bcmd"],
    zip_safe=False,
)
