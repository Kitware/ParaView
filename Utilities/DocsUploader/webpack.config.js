const path = require("path");
const entry = path.resolve(__dirname, "Src", "selector.js");

module.exports = {
  entry,
  output: {
    path: path.resolve(__dirname, "Dist"),
    filename: "paraview-version.js"
  },
  module: {
    rules: [
      {
        test: entry,
        loader: "expose-loader?PV"
      },
      {
        test: /\.js$/,
        use: [{ loader: "babel-loader", options: { presets: ["es2015"] } }]
      },
      {
        test: /\.css$/,
        use: [{ loader: "style-loader" }, { loader: "css-loader" }]
      },
      {
        test: /\.(png|jpg|gif)$/,
        use: [
          {
            loader: "file-loader",
            options: {
              limit: 60000,
            }
          }
        ]
      }
    ]
  }
};
